// SPDX-License-Identifier: MIT
/*
 * TCP transport implementation for RPC server
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#pragma once

#include "server.h"
#include "utils.h"

#include <unistd.h>

#include <thread>


namespace rpc {

class TcpServer : public rpc::Server {
public:
	TcpServer(uint16_t port)
		:listen_sock(tcp::server_socket(port))
	{ }

	~TcpServer()
	{
		close(listen_sock);
	}

	void run(Server& server)
	{
		while (true) {
			int client_sock = accept(listen_sock, nullptr, nullptr);
			if (client_sock < 0)
				continue;

			std::thread([this, client_sock]() {
				try {
					while (true) {
						auto reqBuffer = tcp::recv_buffer(client_sock);

						msgpack::sbuffer buffer;
						buffer.write(reqBuffer.data(), reqBuffer.size());

						auto resp = handle_call(buffer);

						if (resp.size() > 0) {
							tcp::send_buffer(client_sock, resp.data(), resp.size());
						}
					}
				} catch (...) {
					close(client_sock);
				}
			}).detach();
		}
	}

private:
	int listen_sock;
};

}

