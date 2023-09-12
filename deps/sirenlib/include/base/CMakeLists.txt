
find_package(spdlog CONFIG REQUIRED)
link_libraries(spdlog::spdlog)


file(GLOB base_SRCS "*.cpp")
add_library(siren_base ${base_SRCS})

