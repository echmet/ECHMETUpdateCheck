ECHMETUpdateCheck
===

Introduction
---
ECHMETUpdateCheck is a tiny library that fetches a JSON-formatted manifest formatted accordingly to specification described in `format-description.txt` and evaluates if there is an update available. The library features a minimalist C interface, uses [CMake build system](https://cmake.org/) depends on [libcurl](https://curl.haxx.se/libcurl/) and uses the awesome [nlohmann](https://github.com/nlohmann/)'s [json parser](https://github.com/nlohmann/json) to process JSON files. In addition to standard C bindings, bindings for Python using CTypes are available as well.


Build
---

### Linux
Assuming that you have `libcurl` and its development files installed, the building process is very straightforward.

	cd /path/to/ECHMETUpdateCheck
	mkdir build
	cd build
	cmake .. -DCMAKE_BUILD_TYPE=Release
	make
	make install

### Windows
`libcurl` for Windows must be obtained separately before the library can be built. The `LIBCURL_DIR` CMake variable must be set to a path that contains the `libcurl` installation with `lib` and `include` directories inside. CMake can then generate appropriate project files for your compiler of choice. [MinGW64](https://sourceforge.net/projects/mingw-w64/) and MSVC 2015 compilers have been tested to build ECHMETUpdateCheck correctly.

Usage
---
Please refer to the documentation in `include\echmetupdatecheck.h`, `python\echmetupdatecheck.py` and `format-description.txt` for technical details how to interface with the library.

License
---
The library is released under the Lesser GNU GPLv3 software license.
