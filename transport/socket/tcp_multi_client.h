// SPDX-License-Identifier: MIT
/*
 * TCP transport implementation for RPC client.
 * A single client can call multiple servers.
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#pragma once

#include "rpc/client.h"
#include "utils.h"

#include <unistd.h>

#include <thread>


namespace rpc {

class TcpMultiClient {
public:
	TcpMultiClient(const std::string& host, const std::vector<uint16_t>& ports)
	{
		for (auto port : ports) {
			int new_sock = tcp::client_socket(host, port);
			if (new_sock != -1) {
				m_socks.insert(new_sock);
			}
		}
	}

	~TcpMultiClient()
	{
		for (auto sock : m_socks) {
			close(sock);
		}
	}

	template <typename R, typename... Args>
	auto call(const std::string& funcID, Args&&... args)
	{
		auto [future, buffer, id] = m_client.multi_call<R>(funcID, std::forward<Args>(args)...);

		auto transaction = std::async(std::launch::async, [&] {
			for (auto sock : m_socks) {
				tcp::send_buffer(sock, buffer.data(), buffer.size());
			}

			if constexpr (std::is_same_v<R, void>) {
				/* no return value, do not wait */
				return;
			}

			size_t currentSock = 0;
			size_t lastSock = m_socks.size() - 1;
			for (auto sock : m_socks) {
				auto respBuffer = tcp::recv_buffer(sock);

				try {
					msgpack::sbuffer resp;
					resp.write(respBuffer.data(), respBuffer.size());
					m_client.ingest_resp(resp, (currentSock++ == lastSock)/*last*/);
				} catch (...) {
					m_client.cancel(id, std::move(std::current_exception()));
				}
			}
		});

		return future.get();
	}

private:
	std::set<int> m_socks;
	rpc::Client m_client;
};

}

