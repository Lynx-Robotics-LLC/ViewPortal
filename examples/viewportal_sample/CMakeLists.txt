cmake_minimum_required(VERSION 3.14)
project(ViewPortalSampleExample VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use local ViewPortal when built from inside the ViewPortal repo (examples/viewportal_sample -> ../..)
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
    GIT_REPOSITORY https://github.com/your-org/ViewPortal.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(ViewPortal)

find_package(OpenCV REQUIRED)

add_executable(viewportal_sample viewportal_sample.cpp)
target_link_libraries(viewportal_sample PRIVATE viewportal ${OpenCV_LIBS})
target_include_directories(viewportal_sample PRIVATE ${OpenCV_INCLUDE_DIRS})
