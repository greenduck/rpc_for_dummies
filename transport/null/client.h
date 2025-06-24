// SPDX-License-Identifier: MIT
/*
 * Null transport implementation for RPC server - direct call
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#pragma once

#include "rpc/client.h"
#include "rpc/server.h"


namespace rpc {

class NullClient {
public:
	NullClient(rpc::Server& server)
		:m_server(server)
	{ }

	template <typename R, typename... Args>
	R call(const std::string& funcID, Args&&... args)
	{
		auto [future, buffer, id] = m_client.call<R>(funcID, std::forward<Args>(args)...);

		[[maybe_unused]] auto resp = m_server.handle_call(buffer);

		if constexpr (std::is_same_v<R, void>) {
			/* no return value, do not wait */
			return;
		} else {
			try {
				m_client.ingest_resp(resp, true/*last*/);
			} catch (...) {
				m_client.cancel(id, std::move(std::current_exception()));
			}

			return future.get();
		}
	}

private:
    rpc::Client m_client;
    rpc::Server& m_server;
};

}
