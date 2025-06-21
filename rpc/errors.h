// SPDX-License-Identifier: MIT
/*
 * RPC exception class
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#pragma once

#include <stdexcept>


namespace rpc {

class error : public std::runtime_error {
public:
	explicit error(const std::string& msg) noexcept
		: std::runtime_error(msg)
	{ }

	explicit error(const char* msg) noexcept
		: std::runtime_error(msg)
	{ }
};

}
