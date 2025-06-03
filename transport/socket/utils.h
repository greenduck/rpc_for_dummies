#pragma once

#include <cstdint>
#include <string>
#include <vector>


namespace tcp {

int client_socket(const std::string& host, uint16_t port);
int server_socket(uint16_t port);
void send_buffer(int sock, const char* buffer, uint32_t len) noexcept;
std::vector<char> recv_buffer(int sock) noexcept;

}
