cmake_minimum_required(VERSION 3.14)
project(ViewPortalRealsenseExample VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use local ViewPortal when built from inside the ViewPortal repo (examples/realsense -> ../..)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt" _root_cmake)
    if(_root_cmake MATCHES "project\\(ViewPortal ")
        set(FETCHCONTENT_SOURCE_DIR_VIEWPORTAL "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "Use local ViewPortal")
        message(STATUS "Using local ViewPortal from ${FETCHCONTENT_SOURCE_DIR_VIEWPORTAL}")
    endif()
    unset(_root_cmake)
endif()

include(FetchContent)
FetchContent_Declare(
    ViewPortal
    GIT_REPOSITORY https://github.com/Lynx-Robotics-LLC/ViewPortal.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(ViewPortal)

find_package(OpenCV REQUIRED)
find_package(realsense2 REQUIRED)

add_executable(realsense_app realsense_app.cpp)
target_link_libraries(realsense_app PRIVATE
    viewportal
    realsense2::realsense2
    ${OpenCV_LIBS}
)
target_include_directories(realsense_app PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${REALSENSE2_INCLUDE_DIR}
)
