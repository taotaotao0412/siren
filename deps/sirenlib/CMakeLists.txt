
find_package(spdlog CONFIG REQUIRED)
link_libraries(spdlog::spdlog)


file(GLOB_RECURSE base_SRCS "src/*.cpp")
add_library(siren_base ${base_SRCS})
target_include_directories(siren_base PUBLIC include)
target_link_libraries(siren_base PUBLIC pthread)
