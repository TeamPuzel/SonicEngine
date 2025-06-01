
setup:
	@rm -rf build
	@cmake -B build\
	    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON\
		-DCMAKE_CXX_COMPILER="/home/lua/Toolchains/llvm/bin/clang++"

build: setup
	@cd build; make

run: build
	@cd build; ./sonic
