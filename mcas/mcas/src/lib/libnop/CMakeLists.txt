cmake_minimum_required (VERSION 3.5.1 FATAL_ERROR)

project(libnop CXX)

install(DIRECTORY "include/nop" DESTINATION include FILES_MATCHING PATTERN "*")

