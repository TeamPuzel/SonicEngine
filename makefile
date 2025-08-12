
setup:
	@rm -rf build
	@cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja

build: setup
	@cd build; ninja

reload:
	@rm -rf build/CMakeFiles
	@rm -rf build/obj
	@rm -rf build/res
	@rm -rf build/.ninja_deps
	@rm -rf build/.ninja_log
	@rm -rf build/build.ninja
	@rm -rf build/cmake_install.cmake
	@rm -rf build/CMakeCache.txt
	@cmake -B build -DCMAKE_BUILD_TYPE=Release -DHOT_RELOAD=ON -G Ninja
	@cd build; ninja
	@pkill -USR1 sonic

run: build
	@cd build; ./sonic
