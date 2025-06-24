cmake_minimum_required(VERSION 3.18)

project(rpc-test)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS		"-Wall -fmax-errors=1")
set(CMAKE_CXX_FLAGS_DEBUG	"-g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE	"-O2")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(WITH_STANDALONE_TEST "Build unittests into standalone binary" OFF)
option(WITH_TRANSPORT_TEST "Build transport test(s) into standalone binary" OFF)

set(MSGPACK_TAG cpp-7.0.0)
set(GTEST_VERSION 1.14.0)

set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_SOURCE_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/cmake/build_deps")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
	message(STATUS "Download CPM.cmake")
	file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})
include(ExternalProject)

CPMAddPackage(
	NAME msgpack
	GITHUB_REPOSITORY msgpack/msgpack-c
	GIT_TAG ${MSGPACK_TAG}
	OPTIONS "MSGPACK_CXX17 ON" "MSGPACK_BUILD_DOCS OFF" "MSGPACK_USE_BOOST OFF"
)

if(WITH_STANDALONE_TEST)
	CPMAddPackage(
		NAME googletest
		GITHUB_REPOSITORY google/googletest
		VERSION ${GTEST_VERSION}
		OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt" "CMAKE_CXX_CLANG_TIDY "
	)
	set(GTEST_LIBRARY "${CMAKE_BINARY_DIR}/lib")
	set(GTEST_MAIN_LIBRARY "${CMAKE_BINARY_DIR}/lib")
	set(GTEST_INCLUDE_DIR "${googletest_SOURCE_DIR}/googletest/include")
endif()

find_package(GTest)
if (GTest_FOUND)
	message(STATUS "== rpc: Build unittest")
	file(GLOB unittest_files ${CMAKE_CURRENT_SOURCE_DIR}/unittest/*.cpp)
	add_library(rpc_unittest STATIC ${unittest_files})
	target_include_directories(rpc_unittest
		PUBLIC
		  "${CMAKE_CURRENT_SOURCE_DIR}/rpc"
		  "${msgpack_SOURCE_DIR}/include"
	)
	target_link_libraries(rpc_unittest
		PRIVATE
		  msgpack-cxx
	)
else()
	message(STATUS "== rpc: Skip unittest")
endif()

if(WITH_STANDALONE_TEST)
	set(CMAKE_CXX_FLAGS_DEBUG	"-g -DDEBUG")

	target_compile_definitions(rpc_unittest
		PUBLIC
		  MSGPACK_NO_BOOST
	)

	target_include_directories(rpc_unittest
		PRIVATE
		  "${GTEST_INCLUDE_DIR}"
	)

	file(WRITE ${CMAKE_BINARY_DIR}/dummy.cpp "")
	add_executable(unittest ${CMAKE_BINARY_DIR}/dummy.cpp)

	set(rpc_unittest_LINK_LIB -Wl,--whole-archive rpc_unittest -Wl,--no-whole-archive)
	target_link_libraries(unittest PRIVATE gtest gtest_main "${rpc_unittest_LINK_LIB}")
endif()

if(WITH_TRANSPORT_TEST)
	message(STATUS "== rpc: Build transport test")
	file(GLOB tcp_test_files ${CMAKE_CURRENT_SOURCE_DIR}/transport/socket/*.cpp)
	add_executable(tcp_test ${tcp_test_files})
	target_include_directories(tcp_test
		PUBLIC
		  "${CMAKE_CURRENT_SOURCE_DIR}"
		  "${msgpack_SOURCE_DIR}/include"
	)
	target_link_libraries(tcp_test
		PRIVATE
		  msgpack-cxx
		  pthread
	)


	file(GLOB null_test_files ${CMAKE_CURRENT_SOURCE_DIR}/transport/null/*.cpp)
	add_executable(null_test ${null_test_files})
	target_include_directories(null_test
		PUBLIC
		  "${CMAKE_CURRENT_SOURCE_DIR}"
		  "${msgpack_SOURCE_DIR}/include"
	)
	target_link_libraries(null_test
		PRIVATE
		  msgpack-cxx
		  pthread
	)
endif()
