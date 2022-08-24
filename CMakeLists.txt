cmake_minimum_required(VERSION 3.8.2)
project(wxserver)
set(CMAKE_CXX_STANDARD 17)

add_executable(wxserver src/main.cpp)
target_link_libraries (wxserver tinyxml2 curl crypto pthread)
