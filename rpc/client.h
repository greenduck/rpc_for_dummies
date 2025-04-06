// SPDX-License-Identifier: MIT
/*
 * Transport agnostic RPC client
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#pragma once

#include "msgpack.hpp"

#include <unordered_map>
#include <future>
#include <mutex>
#include <atomic>


namespace rpc {

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

			auto wrapper = [=](const msgpack::object& obj) noexcept {
				auto resp = obj.as<R>();

				prom->set_value(std::move(resp));
			};

			/* FIXME: need timeout */
			m_respWaiters.emplace(m_callID, wrapper);
		}

		++m_callID;
		return std::make_tuple(prom->get_future(), std::move(data));
	}

	void ingest_resp(const msgpack::sbuffer& buffer)
	{
		std::vector<msgpack::object> items;

		msgpack::unpack(buffer.data(), buffer.size()).get().convert(items);

		if (items.size() != 2)
			throw std::runtime_error("malformed response buffer");

		auto callID = items[0].as<uint32_t>();
		std::function<void(const msgpack::object&)> wrapper;

		{
			std::lock_guard lock(m_mutex);
			auto it = m_respWaiters.find(callID);

			if (it == m_respWaiters.end())
				throw std::runtime_error("unexpected callID on return: " + callID);

			wrapper = std::move(it->second);
			m_respWaiters.erase(it);
		}

		wrapper(items[1]);
	}

private:
	std::atomic<uint32_t> m_callID = 122333;
	std::unordered_map<uint32_t, std::function<void(const msgpack::object&)>> m_respWaiters;
	std::mutex m_mutex;
};

}

