// SPDX-License-Identifier: MIT
/*
 * Transport agnostic RPC client
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#pragma once

#include "errors.h"

#include "msgpack.hpp"

#include <unordered_map>
#include <future>
#include <mutex>
#include <atomic>
#include <ctime>


namespace rpc {

class ClientError : public error {
public:
	explicit ClientError(const std::string& msg) noexcept
		: error(msg)
	{ }

	explicit ClientError(const char* msg) noexcept
		: error(msg)
	{ }
};


template <typename... Args>
msgpack::sbuffer serialize_call(uint32_t callID, const std::string& funcID, Args&&... args)
{
	auto data = std::make_tuple(callID, funcID, std::forward<Args>(args)...);

	msgpack::sbuffer buffer;
	msgpack::packer<msgpack::sbuffer> packer(buffer);

	packer.pack(data);
	return buffer;
}


class Client {
public:
	template <typename R, typename... Args>
	auto call(const std::string& funcID, Args&&... args)
	{
		auto data = serialize_call(m_callID, funcID, std::forward<Args>(args)...);
		auto prom = std::make_shared<std::promise<R>>();

		if constexpr (std::is_same_v<R, void>) {
			/* no return value, do not wait */
			prom->set_value();
		} else {
			std::lock_guard lock(m_mutex);

			auto wrapper = [=](const msgpack::object& obj, [[maybe_unused]] bool last, std::exception_ptr&& exp) noexcept {
				if (exp != nullptr) {
					prom->set_exception(exp);
					return;
				}

				auto resp = obj.as<R>();

				prom->set_value(std::move(resp));
			};

			m_respWaiters.emplace(m_callID, wrapper);
		}

		return std::make_tuple(prom->get_future(), std::move(data), m_callID++);
	}


	template <typename R, typename... Args>
	auto multi_call(const std::string& funcID, Args&&... args)
	{
		auto data = serialize_call(m_callID, funcID, std::forward<Args>(args)...);

		if constexpr (std::is_same_v<R, void>) {
			/* no return value, do not wait */
			auto prom = std::make_shared<std::promise<R>>();

			prom->set_value();
			return std::make_tuple(prom->get_future(), std::move(data), m_callID++);
		} else {
			auto prom = std::make_shared<std::promise<std::vector<R>>>();
			auto retval = std::make_shared<std::vector<R>>();

			std::lock_guard lock(m_mutex);

			auto wrapper = [=](const msgpack::object& obj, bool last, std::exception_ptr&& exp) noexcept {
				if (exp != nullptr) {
					prom->set_exception(exp);
					return;
				}

				auto resp = obj.as<R>();
				retval->push_back(std::move(resp));

				if (last) {
					prom->set_value(std::move(*retval));
				}
			};

			m_respWaiters.emplace(m_callID, wrapper);
			return std::make_tuple(prom->get_future(), std::move(data), m_callID++);
		}
	}


	/*
	 * Cancellation can be invoked, e.g. upon timeout.
	 * The exception will be thrown by the `future`.
	 */
	template<typename ExceptionT>
	bool cancel(uint32_t callID, ExceptionT&& ex) noexcept
	{
		return cancel(callID, std::make_exception_ptr(ex));
	}

	bool cancel(uint32_t callID, std::exception_ptr&& exp) noexcept
	{
		std::function<void(const msgpack::object&, bool, std::exception_ptr&&)> wrapper;

		{
			std::lock_guard lock(m_mutex);
			auto it = m_respWaiters.find(callID);

			if (it == m_respWaiters.end())
				return false;

			wrapper = std::move(it->second);
			m_respWaiters.erase(it);
		}

		wrapper({}, true, std::move(exp));
		return true;
	}


	void ingest_resp(const msgpack::sbuffer& buffer, bool last = true)
	{
		std::vector<msgpack::object> items;

		msgpack::unpack(buffer.data(), buffer.size()).get().convert(items);

		if (items.size() != 2)
			throw ClientError("malformed response buffer");

		auto callID = items[0].as<uint32_t>();
		std::function<void(const msgpack::object&, bool, std::exception_ptr&&)> wrapper;

		{
			std::lock_guard lock(m_mutex);
			auto it = m_respWaiters.find(callID);

			if (it == m_respWaiters.end())
				throw ClientError("unexpected callID on return: " + std::to_string(callID));

			if (last) {
				wrapper = std::move(it->second);
				m_respWaiters.erase(it);
			} else {
				wrapper = it->second;
			}
		}

		wrapper(items[1], last, nullptr);
	}

private:
	std::atomic<uint32_t> m_callID{static_cast<uint32_t>(std::time(nullptr))};
	std::unordered_map<uint32_t, std::function<void(const msgpack::object&, bool, std::exception_ptr&&)>> m_respWaiters;
	std::mutex m_mutex;
};

}

