# A makefile containing some commands I use a lot.
# It is not actually used for building anything, all of that is specified with CMake.
# These are mostly used as Zed tasks but I define them here for compatibility with other workflows.
#
# You will find that it's just configured for release builds, that's because my code is
# largely pure and I only need the debugger every few weeks and my code relies on optimizations.
# The graphics code in particular is a difference of orders of magnitude.

all: setup

# Counts the lines of code :)
# Requires cloc to be installed of course.
cloc:
	@cloc src object tools

# Switch clangd to the native build.
clangd-build:
	@echo "CompileFlags:" > .clangd
	@echo "  CompilationDatabase: build" >> .clangd

# Switch clangd to the cross-build.
clangd-cross:
	@echo "CompileFlags:" > .clangd
	@echo "  CompilationDatabase: build-cross" >> .clangd

setup: clangd-build
	@rm -rf build
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build: setup
	@cd build; ninja

# A simple command for recompiling parts of the game while it's running.
reload:
	@rm -rf build/CMakeFiles
	@rm -rf build/obj
	@rm -rf build/res
	@rm -rf build/.ninja_deps
	@rm -rf build/.ninja_log
	@rm -rf build/build.ninja
	@rm -rf build/cmake_install.cmake
	@rm -rf build/CMakeCache.txt
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DHOT_RELOAD=ON
	@cd build; ninja
	@pkill -USR1 sonic

# Runs the game natively.
run: build
	@cd build; ./sonic

# A convenience for building the binary for Windows from UNIX operating systems.
# It's not even that hard, I feel bad for people who think they need to use Windows for anything.
# If anything supporting MSVC is more difficult due to how different it is and how sad the C++ standard is.
cross-setup: clangd-cross
	@rm -rf build-cross
	@cmake -B build-cross -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON   \
	    -DCMAKE_TOOLCHAIN_FILE=windows/toolchain.cmake -DCMAKE_CXX_FLAGS="-DCROSS_COMPILING=1"

cross-build: cross-setup
	@cd build-cross; ninja

cross-run: cross-build
	@cd build-cross; wine sonic.exe
