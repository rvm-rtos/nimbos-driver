ARCH ?= x86_64
MODE ?= release

cmake_build_args := -DARCH=$(ARCH)
ifeq ($(MODE), release)
  cmake_build_args += -DCMAKE_BUILD_TYPE=Release
else ifeq ($(MODE), debug)
  cmake_build_args += -DCMAKE_BUILD_TYPE=Debug
endif

build:
	@echo Building C user app
	@mkdir -p build
	@cd build && cmake $(cmake_build_args) ../ && make -j

clean:
	@rm -rf build

.PHONY: build clean
