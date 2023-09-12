  
file(GLOB_RECURSE net_SRCS "*.cpp")
add_library(siren_net ${net_SRCS} )

target_link_libraries(siren_net siren_base)

