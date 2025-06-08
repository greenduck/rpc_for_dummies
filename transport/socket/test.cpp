#include "tcp_client.h"
#include "tcp_multi_client.h"
#include "tcp_server.h"

#include <iostream>


static void rpc_client(int argc, char* argv[])
{
	uint16_t port = 5555;

	if (argc > 2) {
		try {
			port = std::stoi(argv[2]);
		} catch (const std::exception& ex) {
			std::cerr << "Invalid port number: " << argv[2] << "\n";
			return;
		}
	}

	try {
		rpc::TcpClient client("127.0.0.1", port);

		int result = client.call<int>("add", 3, 4);
		std::cout << "Result: " << result << "\n";

		client.call<void>("print", "Hello, world !");
	} catch (const std::exception& ex) {
		std::cerr << "RPC call failed: " << ex.what() << "\n";
	} catch (...) {
		std::cerr << "RPC call failed\n";
	}
}

static void rpc_multi_client(int argc, char* argv[])
{
	std::vector<uint16_t> port{5555};

	if (argc > 2) {
		try {
			port.clear();

			for (int i = 2; i < argc; ++i) {
				port.push_back(static_cast<uint16_t>(std::stoi(argv[i])));
			}
		} catch (const std::exception& ex) {
			std::cerr << "Invalid port number: " << argv[2] << "\n";
			return;
		}
	}

	try {
		rpc::TcpMultiClient client("127.0.0.1", port);

		std::vector<int> result = client.call<int>("add", 4, 5);

		std::cout << "Result:\n";
		for (const auto& res : result)
			std::cout << "  " << res << "\n";

		client.call<void>("print", "Hello, many worlds !");
	} catch (const std::exception& ex) {
		std::cerr << "RPC call failed: " << ex.what() << "\n";
	} catch (...) {
		std::cerr << "RPC call failed\n";
	}
}

static void rpc_server(int argc, char* argv[])
{
	uint16_t port = 5555;

	if (argc > 2) {
		try {
			port = std::stoi(argv[2]);
		} catch (const std::exception& ex) {
			std::cerr << "Invalid port number: " << argv[2] << "\n";
			return;
		}
	}

	try {
		rpc::TcpServer server(port);

		server.bind("add", [](int a, int b) {
			return a + b;
		});

		server.bind("print", [](std::string msg) {
			std::cout << ">> " << msg << "\n";
		});

		server.run(server);
	} catch (const std::exception& ex) {
		std::cerr << "RPC server failed: " << ex.what() << "\n";
	} catch (...) {
		std::cerr << "RPC server failed\n";
	}
}

int main(int argc, char* argv[])
{
	if ((argc > 1) && (std::string(argv[1]) == "--server")) {
		rpc_server(argc, argv);
		return 0;
	}

	if ((argc > 1) && (argc < 4) && (std::string(argv[1]) == "--client")) {
		/* 1 port - implicit or explicit */
		rpc_client(argc, argv);
		return 0;
	}

	if ((argc > 3) && (std::string(argv[1]) == "--client")) {
		/* 2+ ports specified */
		rpc_multi_client(argc, argv);
		return 0;
	}

	std::cerr << "TCP RPC test\n";
	std::cerr << "Command line options:\n";
	std::cerr << "  --server [port]                      invoke RPC server\n";
	std::cerr << "  --client [port [port [port [...]]]]  invoke RPC client\n";
	return 1;
}
