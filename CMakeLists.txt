# cmake_minimum_required(VERSION 3.16.0)
# include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# project(freeVitals)
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/src")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mila)
message("cmake for vitalsFree")