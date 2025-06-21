// SPDX-License-Identifier: MIT
/*
 * Transport agnostic RPC server
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#pragma once

#include "errors.h"

#include "msgpack.hpp"

#include <shared_mutex>
#include <mutex>
#include <functional>
#include <type_traits>


/*
 * Helper to extract lambda argument types as a tuple
 */
/* Base template: unsupported type */
template <typename T>
struct function_traits;

/* Specialization for std::function */
template <typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> {
	using return_type = Ret;
	using args_tuple = std::tuple<Args...>;
};

/* Specialization for function pointers */
template <typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> {
	using return_type = Ret;
	using args_tuple = std::tuple<Args...>;
};

/* Specialization for member function pointers (like lambdas) */
template <typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret(ClassType::*)(Args...) const> {
	using return_type = Ret;
	using args_tuple = std::tuple<Args...>;
};

/* General callable/lambda/functor case */
template <typename T>
struct function_traits {
private:
	using call_type = function_traits<decltype(&T::operator())>;

public:
	using return_type = typename call_type::return_type;
	using args_tuple = typename call_type::args_tuple;
};


namespace rpc {

class ServerError : public error {
public:
	explicit ServerError(const std::string& msg) noexcept
		: error(msg)
	{ }

	explicit ServerError(const char* msg) noexcept
		: error(msg)
	{ }
};


class Server {
public:
	template <typename Func>
	void bind(const std::string& funcID, Func&& func) noexcept
	{
		std::unique_lock lock(m_mutex);

		auto wrapper = [=](uint32_t callID, const msgpack::sbuffer& buffer) -> msgpack::sbuffer {
			using Traits = function_traits<std::decay_t<Func>>;
			using ArgTuple = typename Traits::args_tuple;
			using RetType = typename Traits::return_type;

			using FullTuple = decltype(std::tuple_cat(std::make_tuple(uint32_t(0), std::string()), ArgTuple()));
			FullTuple full;

			msgpack::unpack(buffer.data(), buffer.size()).get().convert(full);

			msgpack::sbuffer resp;
			msgpack::packer<msgpack::sbuffer> packer(resp);

			std::apply([&](auto&&, auto&&, auto&&... args) {
				if constexpr (std::is_same_v<RetType, void>) {
					/* no return value */
					func(std::forward<decltype(args)>(args)...);
				} else {
					/* return value */
					auto val = func(std::forward<decltype(args)>(args)...);
					packer.pack_array(2);
					packer.pack(callID);
					packer.pack(val);
				}
			}, full);

			return resp;
		};

		m_callbacks.emplace(funcID, wrapper);
	}

	void unbind(const std::string& funcID) noexcept
	{
		std::unique_lock lock(m_mutex);

		m_callbacks.erase(funcID);
	}

	msgpack::sbuffer handle_call(const msgpack::sbuffer& buffer)
	{
		std::shared_lock lock(m_mutex);

		auto [callID, funcID] = get_id(buffer);
		auto callback = m_callbacks.find(funcID);

		if (callback == m_callbacks.end())
			throw ServerError("unregistered function: " + funcID);

		/* FIXME: handle exceptions */
		return callback->second(callID, buffer);
	}

private:
	std::unordered_map<std::string, std::function<msgpack::sbuffer(uint32_t, const msgpack::sbuffer&)>> m_callbacks;
	std::shared_mutex m_mutex;

	std::tuple<uint32_t, std::string> get_id(const msgpack::sbuffer& buffer)
	{
		std::tuple<uint32_t, std::string> id;

		msgpack::unpack(buffer.data(), buffer.size()).get().convert(id);
		return id;
	}
};

}

