cmake_minimum_required(VERSION 3.13)
project(hux-cost-counter LANGUAGES C CXX)

find_package(LLVM 14 REQUIRED CONFIG)
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(HuxCostCounter MODULE CostCounterPass.cpp)
target_include_directories(HuxCostCounter SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(HuxCostCounter PRIVATE ${LLVM_DEFINITIONS})
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(HuxCostCounter PRIVATE -fno-rtti)
endif()
target_compile_options(HuxCostCounter PRIVATE -Wall -Wextra)

install(TARGETS HuxCostCounter LIBRARY DESTINATION lib)
