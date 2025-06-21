// SPDX-License-Identifier: MIT
/*
 * Transport agnostic RPC: unittest
 *
 * Copyright (c) 2025, Andrey Gelman <andrey.gelman@gmail.com>
 */

#include <gtest/gtest.h>

#include "client.h"
#include "server.h"

#include <tuple>
#include <thread>
#include <chrono>


using namespace std::chrono_literals;


static double add(double a, double b)
{
	return a + b;
}

class RPCTest : public ::testing::Test {
protected:
	rpc::Server server;
	rpc::Client client;

	void SetUp() override
	{
		/* Bind functions to the server */
		server.bind("add", add);

		server.bind("sub", [](double a, double b) -> double {
			return a - b;
		});

		server.bind("zero", []() -> double {
			return 0.0;
		});

		server.bind("div", [](double a, double b) -> std::tuple<bool, double> {
			if (b == 0.0)
				return std::make_tuple(false, 0.0);

			return std::make_tuple(true, (a / b));
		});
	}
};

TEST_F(RPCTest, ArithTest)
{
	auto [fut1, buff1, id1] = client.call<double>("add", 90, 21);
	auto [fut2, buff2, id2] = client.call<double>("sub", 123, 12);
	ASSERT_GT(id2, id1);

	auto respBuff1 = server.handle_call(buff1);
	auto respBuff2 = server.handle_call(buff2);

	client.ingest_resp(respBuff2);
	client.ingest_resp(respBuff1);
	EXPECT_EQ(fut1.get(), 111);
	EXPECT_EQ(fut2.get(), 111);
}

TEST_F(RPCTest, DivisionByZeroTest)
{
	auto [fut, buff, _] = client.call<std::tuple<bool, double>>("div", 24, 0);

	auto respBuff = server.handle_call(buff);

	client.ingest_resp(respBuff);
	auto [valid, value] = fut.get();
	EXPECT_FALSE(valid);
	EXPECT_EQ(value, 0.0);
}

TEST_F(RPCTest, DivisionValidTest)
{
	auto [fut, buff, _] = client.call<std::tuple<bool, double>>("div", 24, 3);

	auto respBuff = server.handle_call(buff);

	client.ingest_resp(respBuff);
	auto [valid, value] = fut.get();
	EXPECT_TRUE(valid);
	EXPECT_EQ(value, 8.0);
}

TEST_F(RPCTest, ZeroFunctionTest)
{
	auto [fut, buff, _] = client.call<double>("zero");

	auto respBuff = server.handle_call(buff);

	client.ingest_resp(respBuff);
	EXPECT_EQ(fut.get(), 0.0);
}

TEST_F(RPCTest, ReturnVoidTest)
{
	auto [fut, buff, _] = client.call<void>("trigger", 3);

	auto status = fut.wait_for(0s);
	EXPECT_EQ(status, std::future_status::ready);


	int count = 0;

	server.bind("trigger", [&](int delta) {
		count += delta;
	});

	auto respBuff = server.handle_call(buff);
	EXPECT_EQ(respBuff.size(), 0UL);
	EXPECT_EQ(count, 3);

	server.unbind("trigger");
}

TEST_F(RPCTest, MultiCallTest)
{
	int count = 0;

	server.bind("trigger", [&](int delta) -> int {
		count += delta;

		return count;
	});

	auto [fut, buff, _] = client.multi_call<int>("trigger", 3);

	auto respBuff1 = server.handle_call(buff);
	auto respBuff2 = server.handle_call(buff);
	auto respBuff3 = server.handle_call(buff);

	client.ingest_resp(respBuff1, false);
	EXPECT_EQ(fut.wait_for(0s), std::future_status::timeout);

	client.ingest_resp(respBuff2, false);
	EXPECT_EQ(fut.wait_for(0s), std::future_status::timeout);

	client.ingest_resp(respBuff3, true/*last*/);
	EXPECT_EQ(fut.wait_for(0s), std::future_status::ready);

	std::vector<int> refVec{3, 6, 9};
	EXPECT_EQ(fut.get(), refVec);

	server.unbind("trigger");
}

TEST_F(RPCTest, ReturnVoidMultiCallTest)
{
	int count = 0;

	server.bind("trigger", [&](int delta) {
		count += delta;
	});

	auto [fut, buff, _] = client.multi_call<void>("trigger", 3);

	EXPECT_EQ(fut.wait_for(0s), std::future_status::ready);

	auto respBuff1 = server.handle_call(buff);
	auto respBuff2 = server.handle_call(buff);
	auto respBuff3 = server.handle_call(buff);

	EXPECT_EQ(respBuff1.size(), 0UL);
	EXPECT_EQ(respBuff2.size(), 0UL);
	EXPECT_EQ(respBuff3.size(), 0UL);
	EXPECT_EQ(count, 9);

	server.unbind("trigger");
}

TEST_F(RPCTest, CancellationTest)
{
	auto [fut1, buff1, id1] = client.call<double>("add", 90, 21);
	auto [fut2, buff2, id2] = client.call<double>("sub", 123, 12);
	ASSERT_GT(id2, id1);

	auto respBuff1 = server.handle_call(buff1);
	auto respBuff2 = server.handle_call(buff2);

	/* can be invoked, e.g. upon timeout */
	client.cancel(id1, std::underflow_error("test cancellation"));

	EXPECT_THROW(client.ingest_resp(respBuff1), rpc::ClientError);
	EXPECT_NO_THROW(client.ingest_resp(respBuff2));

	EXPECT_THROW(fut1.get(), std::underflow_error);
	EXPECT_EQ(fut2.get(), 111);
}
