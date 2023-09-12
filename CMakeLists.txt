cmake_minimum_required(VERSION 3.1)

project(siren C CXX)

set(CXX_FLAGS
 -g
 -std=c++17
 -pthread
#  -O3
 )
#  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_BUILD_TYPE Debug)


set(APP_NAME "main")

add_executable(${APP_NAME} 
./test/main.cpp
)

include_directories(${PROJECT_SOURCE_DIR})
# add_subdirectory(siren/base)
add_subdirectory(deps/sirenlib)

  
file(GLOB_RECURSE net_SRCS "src/*.cpp")
add_library(siren_net ${net_SRCS} )
target_include_directories(siren_net PUBLIC include)

target_link_libraries(siren_net siren_base)



# add_subdirectory(siren/net)
add_subdirectory(example)
target_link_libraries(${APP_NAME} PUBLIC siren_net)
  



# target_include_directories(main PUBLIC ${PROJECT_SOURCE_DIR})




# set (DEPS_DIR
# /app/env_ubuntu18.04_cuda10.1
# )
# set( DEPS_INCLUDE
# ${DEPS_DIR}/spdlog/include
# )
# link_directories(${DEPS_DIR}/spdlog/lib)
# target_include_directories(${APP_NAME} PUBLIC ${DEPS_INCLUDE})

# # pthread
# target_link_libraries(${APP_NAME} PRIVATE pthread)

# vcpkg
# spdlog
# find_package(spdlog CONFIG REQUIRED)
# target_link_libraries(${APP_NAME}  PRIVATE spdlog::spdlog)

# fmt
# find_package(fmt CONFIG REQUIRED)
# target_link_libraries(${APP_NAME}  PRIVATE fmt::fmt)