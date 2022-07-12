cmake_minimum_required(VERSION 3.22)
project(wxserver)
set(CMAKE_CXX_STANDARD 17)

add_executable(wxserver main.cpp)
target_link_libraries (wxserver tinyxml2 curl crypto opencv_videoio opencv_core opencv_imgcodecs)
