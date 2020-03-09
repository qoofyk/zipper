add_executable(socket_server server.c)
add_executable(socket_client client.c)

target_link_libraries(socket_server zipper_utils)
target_link_libraries(socket_client zipper_utils)