#include "utils.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <stdexcept>


namespace tcp {

int client_socket(const std::string& host, uint16_t port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		throw std::runtime_error("socket() failed");

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
		throw std::runtime_error("inet_pton failed");

	if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("connect() failed");

	return sock;
}

int server_socket(uint16_t port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		throw std::runtime_error("socket() failed");

	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("bind() failed");

	if (listen(sock, 5) < 0)
		throw std::runtime_error("listen() failed");

	return sock;
}

void send_buffer(int sock, const char* buffer, uint32_t len) noexcept
{
	uint32_t net_len = htonl(len);

	send(sock, &net_len, sizeof(net_len), 0);
	send(sock, buffer, len, 0);
}

std::vector<char> recv_buffer(int sock) noexcept
{
	uint32_t net_len = 0;
	recv(sock, &net_len, sizeof(net_len), MSG_WAITALL);

	uint32_t len = ntohl(net_len);
	std::vector<char> buffer(len);

	recv(sock, buffer.data(), len, MSG_WAITALL);
	return buffer;
}

}
