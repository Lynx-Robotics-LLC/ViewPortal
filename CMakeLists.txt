cmake_minimum_required(VERSION 3.14)
project(ViewPortal VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies (Pangolin only; apps/examples bring OpenCV/RealSense as needed) ---
find_package(Pangolin QUIET)
if(NOT Pangolin_FOUND)
    message(STATUS "Pangolin not found, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        pangolin
        GIT_REPOSITORY https://github.com/stevenlovegrove/Pangolin.git
        GIT_TAG master
    )
    set(BUILD_PANGOLIN_PYTHON OFF CACHE BOOL "" FORCE)
    set(BUILD_PANGOLIN_APPLE OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(pangolin)
    message(STATUS "Pangolin fetched and built via FetchContent")
else()
    message(STATUS "Pangolin found: ${Pangolin_VERSION}")
endif()

# --- ViewPortal library (consumable via FetchContent) ---
add_library(viewportal
    src/viewportal_display.cpp
    src/viewportal_params.cpp
    src/viewport_factory.cpp
    src/viewport_rgb8.cpp
    src/viewport_g8.cpp
    src/viewport_colored_depth.cpp
    src/viewport_reconstruction.cpp
    src/viewport_plot.cpp
)

target_include_directories(viewportal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(TARGET pangolin)
    target_link_libraries(viewportal PUBLIC pangolin)
elseif(TARGET pango_display)
    if(Pangolin_LIBRARIES)
        target_link_libraries(viewportal PUBLIC ${Pangolin_LIBRARIES})
    else()
        target_link_libraries(viewportal PUBLIC
            pango_display pango_opengl pango_core pango_vars
            pango_image pango_plot pango_windowing
        )
    endif()
else()
    target_link_libraries(viewportal PUBLIC ${Pangolin_LIBRARIES})
    target_include_directories(viewportal PRIVATE ${Pangolin_INCLUDE_DIRS})
endif()

# --- Install (optional; for install-tree consumption) ---
install(TARGETS viewportal
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include
    FILES_MATCHING PATTERN "*.h")
install(FILES config/params.cfg DESTINATION share/viewportal/config)
