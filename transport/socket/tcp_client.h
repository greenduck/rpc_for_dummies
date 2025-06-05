// SPDX-License-Identifier: MIT
/*
 * TCP transport implementation for RPC client
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#pragma once

#include "client.h"
#include "utils.h"

#include <unistd.h>

#include <thread>


namespace rpc {

class TcpClient {
public:
	TcpClient(const std::string& host, uint16_t port)
		:m_sock(tcp::client_socket(host, port))
	{ }

	~TcpClient()
	{
		close(m_sock);
	}

	template <typename R, typename... Args>
	R call(const std::string& funcID, Args&&... args)
	{
		auto [future, buffer, id] = m_client.call<R>(funcID, std::forward<Args>(args)...);

		auto transaction = std::async(std::launch::async, [&] {
			tcp::send_buffer(m_sock, buffer.data(), buffer.size());

			if constexpr (std::is_same_v<R, void>) {
				/* no return value, do not wait */
				return;
			}

			auto respBuffer = tcp::recv_buffer(m_sock);
			if (respBuffer.empty()) {
				m_client.cancel(id, std::runtime_error("client: no response"));
				return;
			}

			try {
				msgpack::sbuffer resp;
				resp.write(respBuffer.data(), respBuffer.size());
				m_client.ingest_resp(resp, true/*last*/);
			} catch (...) {
				m_client.cancel(id, std::move(std::current_exception()));
			}
		});

		return future.get();
	}

private:
	int m_sock;
	rpc::Client m_client;
};

}

