
setup:
	@rm -rf build
	@cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build: setup
	@cd build; make

run: build
	@cd build; ./sonic
