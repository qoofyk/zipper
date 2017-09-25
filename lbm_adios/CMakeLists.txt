cmake_minimum_required(VERSION 2.8)
project("transport testing")

add_library(transport transports.c)

add_subdirectory(test_tranports)
