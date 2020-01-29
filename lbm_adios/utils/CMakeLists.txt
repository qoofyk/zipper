if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DDEBUG)
endif()
add_library(zipper_utils conn_socket.c)
