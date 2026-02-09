cmake_minimum_required(VERSION 3.10)
project(WasmEdgeDepManager)

set(CMAKE_CXX_STANDARD 17)

# Manual path configuration since CMake config files are missing 
set(WASMEDGE_INCLUDE_DIR "$ENV{HOME}/.wasmedge/include")
set(WASMEDGE_LIB_DIR "$ENV{HOME}/.wasmedge/lib")

# Include the local include folder and the WasmEdge SDK include folder 
include_directories(include src ${WASMEDGE_INCLUDE_DIR})

# Tell the linker where to find libwasmedge.so 
link_directories(${WASMEDGE_LIB_DIR})

add_executable(dep_manager src/main.cpp)

# Link against the shared library file directly 
target_link_libraries(dep_manager PRIVATE wasmedge)

# Ensure the executable knows where the library is at runtime
set_target_properties(dep_manager PROPERTIES INSTALL_RPATH "${WASMEDGE_LIB_DIR}")
set_target_properties(dep_manager PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)