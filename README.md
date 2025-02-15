# Aerospike C Client

The Aerospike C client provides a C interface for interacting with the 
[Aerospike](http://aerospike.com) Database.  Examples and unit tests are
also included.

## Build Prerequisites

The C client can be built on most recent 64-bit Linux distributions
and Mac OS X 10.8 or greater.  The build requires gcc 4.1 or newer 
for Linux and XCode clang for Mac OS X.

### Debian 6+ and Ubuntu 12+

	$ sudo apt-get install libc6-dev libssl-dev autoconf automake libtool g++

	[Also do on Ubuntu 12+:]
	$ sudo apt-get install ncurses-dev

	[Optional:]
	$ sudo apt-get install liblua5.1-dev

### Red Hat Enterprise Linux or CentOS 6+

	$ sudo yum install openssl-devel glibc-devel autoconf automake libtool

	[Optional:]
	$ sudo yum install lua-devel
	$ sudo yum install gcc-c++ graphviz rpm-build 

### Fedora 20+

	$ sudo yum install openssl-devel glibc-devel autoconf automake libtool

	[Optional:]
	$ sudo yum install compat-lua-devel-5.1.5
	$ sudo yum install gcc-c++ graphviz rpm-build 

### Mac OS X

	Download and install [XCode](https://itunes.apple.com/us/app/xcode/id497799835).

## Build

Before building, please ensure you have the prerequisites installed.  This project uses 
git submodules, so you will need to initialize and update submodules before building 
this project.

	$ git submodule update --init

To build:

	$ make

This will generate the following files:

- `target/{target}/include` – header files
- `target/{target}/lib/libaerospike.a` – static archive
- `target/{target}/lib/libaerospike.so` – dynamic shared library (for Linux)
  **or**
- `target/{target}/lib/libaerospike.dylib` – dynamic shared library (for MacOS)

Static linking with the `.a` prevents you from having to install the libraries on your 
target platform. Dynamic linking with the `.so` avoids a client rebuild if you upgrade 
the client. Choose the option that is right for you.

## Clean

To clean up build products:

	$ make clean

This will remove all files in the `target` directory.

## Test

To run unit tests:

	$ make [AS_HOST=<hostname>] test

or with valgrind:

	$ make [AS_HOST=<hostname>] test-valgrind

## Lua

The C client requires [Lua](http://www.lua.org) 5.1 support for the
client-side portion of User Defined Function (UDF) query aggregation.
By default, the C client builds with Lua support provided by the
included `lua` submodule.

Optionally, Lua support may be provided by either the included `luajit`
submodule or by the build environment.

To enable [LuaJIT](http://luajit.org) 2.0.3, the build must be performed
with the `USE_LUAJIT=1` option passed on all relevant `make` command
lines (i.e., the C client itself, the benchmarks sample application, and
the API examples.) [Note that on some platforms, [Valgrind](http://www.valgrind.org)
may not function out-of-the-box on applications built with the C client
when LuaJIT is enabled without using an unreleased version of LuaJIT
built with additional options.]

To use Lua provided by the development environment, either the `lua5.1`
development package may be installed (on platforms that have it), or
else Lua 5.1.5 may be built from the source release and installed into
the standard location (usually `/usr/local/`.) In either of these two
cases, the build must be performed with the option `USE_LUAMOD=0` passed
on all relevant `make` command lines.

## Package

Installer packages can be created for RedHat (rpm), Debian (deb), Mac OS X (pkg).
These packages contain C client libraries, header files, online docs, examples and 
benchmarks.  Package creation requires doxygen 1.8 or greater and its dependencies
(including graphviz).  Doxygen is used to create online HTML documentation.

Build the client package on the current platform:

	$ make package

The generated packages are located in `target/packages`.

