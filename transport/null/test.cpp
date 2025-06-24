// SPDX-License-Identifier: MIT
/*
 * Null transport implementation for RPC server - direct call
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#include "client.h"

#include <iostream>


int main()
{
	rpc::Server server;

	server.bind("print", [](std::string msg) {
		std::cout << ">> " << msg << "\n";
	});

	server.bind("add", [](int a, int b) {
		return a + b;
	});

	try {
		rpc::NullClient client(server);

		client.call<void>("print", "Hello, world !");

		int result = client.call<int>("add", 3, 4);
		std::cout << "Result: " << result << "\n";

	} catch (const rpc::ServerError& ex) {
		std::cerr << "RPC call failed: " << ex.what() << "\n";
	} catch (...) {
		std::cerr << "RPC call failed\n";
	}

	return 0;
}
