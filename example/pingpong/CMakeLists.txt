add_executable(pingpong_client client.cc)
target_link_libraries(pingpong_client siren_net)

add_executable(pingpong_server server.cc)
target_link_libraries(pingpong_server siren_net)

add_executable(pingpong_bench bench.cc)
target_link_libraries(pingpong_bench siren_net)
