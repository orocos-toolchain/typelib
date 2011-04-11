ifdef ROS_ROOT
default: install
include $(shell rospack find mk)/cmake.mk
export PKG_CONFIG_PATH:=$(shell rospack find utilmm)/install/lib/pkgconfig
EXTRA_CMAKE_FLAGS=-DCMAKE_INSTALL_PREFIX=`rospack find typelib`/install\
                  -DLIBRARY_OUTPUT_PATH=`rospack find typelib`/lib

ifdef ROS_STACK_DIR_FINAL
EXTRA_CMAKE_FLAGS +=-DTYPELIB_PLUGIN_PATH="${ROS_STACK_DIR_FINAL}/orocos_toolchain_ros/typelib/install/lib/typelib"
endif

install: all
	mkdir -p build && cd build && cmake .. && ${MAKE} install
else
$(warning This Makefile only works with ROS rosmake. Without rosmake, create a build directory and run cmake ..)
endif
