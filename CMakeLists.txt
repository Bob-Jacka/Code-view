cmake_minimum_required(VERSION 3.31)
project(code_inspector)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(CMAKE_DEFAULT_BUILD_TYPE "Debug" CACHE STRING "" FORCE)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED
)

add_executable(code_inspector
        main.cpp
        Interface.hpp
)

target_link_libraries(code_inspector
        PRIVATE
        Qt6::Core
        Qt6::Widgets
)
