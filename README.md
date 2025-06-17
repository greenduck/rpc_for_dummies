# RPC for dummies

## Overview

`RFD` (RPC For Dummies) is a header-only RPC library for recent C++ providing both server and client implementations.

Main highlights:

- No IDL
- Asynchronous
- Supports single client / multiple servers
- Transport agnostic

This library is inspired by [rpclib for C++](https://github.com/rpclib/rpclib).

## License

This software is licensed under MIT license.

## Building

Normally `RFD` (including its unittests) is supposed to be integrated with the user's application,
where it will be integrated with the transport layer of choice.

However, it is possible to exercise a local build that will produce stand-alone unittest binary,
and an example application with TCP/IP transport:

```sh
$ mkdir build && cd $_
$ cmake -DWITH_STANDALONE_TEST=ON -DWITH_TRANSPORT_TEST=ON ../
$ make
$ ls -l
...
transport_test
unittest
```
