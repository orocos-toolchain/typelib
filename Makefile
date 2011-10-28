ifdef ROS_ROOT
default: install
include $(shell rospack find mk)/cmake.mk
include $(shell rospack find rtt)/../env.mk
EXTRA_CMAKE_FLAGS=-DCMAKE_INSTALL_PREFIX=`rospack find rtt`/../install\
                  -DLIBRARY_OUTPUT_PATH=`rospack find typelib`/lib

ifdef ROS_STACK_DIR_FINAL
EXTRA_CMAKE_FLAGS +=-DTYPELIB_PLUGIN_PATH="${ROS_STACK_DIR_FINAL}/orocos_toolchain/install/lib/typelib"
endif

install: all
	mkdir -p build && cd build && cmake .. && ${MAKE} install
else
$(warning This Makefile only works with ROS rosmake. Without rosmake, create a build directory and run cmake ..)
endif
