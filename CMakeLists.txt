cmake_minimum_required(VERSION 3.14)
project(ViewPortal VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find Pangolin first
find_package(Pangolin QUIET)

# If Pangolin is not found, use FetchContent to get it
if(NOT Pangolin_FOUND)
    message(STATUS "Pangolin not found, fetching from GitHub...")
    
    include(FetchContent)
    FetchContent_Declare(
        pangolin
        GIT_REPOSITORY https://github.com/stevenlovegrove/Pangolin.git
        GIT_TAG master
    )
    
    # Configure Pangolin build options
    set(BUILD_PANGOLIN_PYTHON OFF CACHE BOOL "" FORCE)
    set(BUILD_PANGOLIN_APPLE OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(pangolin)
    
    # After FetchContent populates, get the source directory
    FetchContent_GetProperties(pangolin SOURCE_DIR PANGOLIN_SOURCE_DIR)
    if(PANGOLIN_SOURCE_DIR)
        message(STATUS "Pangolin source directory: ${PANGOLIN_SOURCE_DIR}")
    endif()
    
    message(STATUS "Pangolin fetched and built via FetchContent")
else()
    message(STATUS "Pangolin found: ${Pangolin_VERSION}")
endif()

# Create executable (simworld-style: app + factory + viewport sources)
add_executable(${PROJECT_NAME}
    apps/viewportal_app.cpp
    src/viewportal_display.cpp
    src/viewportal_params.cpp
    src/viewport_factory.cpp
    src/viewport_rgb8.cpp
    src/viewport_g8.cpp
    src/viewport_colored_depth.cpp
    src/viewport_reconstruction.cpp
    src/viewport_plot.cpp
)

# Set include directory for headers
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Find OpenCV
find_package(OpenCV REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})

# Link against Pangolin
# Use the pangolin target if available, otherwise fall back to old-style linking
if(TARGET pangolin)
    target_link_libraries(${PROJECT_NAME} PRIVATE pangolin)
    get_target_property(PANGOLIN_INC pangolin INTERFACE_INCLUDE_DIRECTORIES)
    if(PANGOLIN_INC)
        target_include_directories(${PROJECT_NAME} PRIVATE ${PANGOLIN_INC})
    endif()
elseif(TARGET pango_display)
    # Link all Pangolin components so we get all required includes and libs
    if(Pangolin_LIBRARIES)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${Pangolin_LIBRARIES})
        foreach(_pango_lib ${Pangolin_LIBRARIES})
            if(TARGET ${_pango_lib})
                get_target_property(_inc ${_pango_lib} INTERFACE_INCLUDE_DIRECTORIES)
                if(_inc)
                    target_include_directories(${PROJECT_NAME} PRIVATE ${_inc})
                endif()
            endif()
        endforeach()
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE pango_display pango_opengl pango_core pango_vars pango_image pango_plot pango_windowing)
        foreach(_pango_lib pango_display pango_opengl pango_core pango_vars pango_image pango_plot pango_windowing)
            if(TARGET ${_pango_lib})
                get_target_property(_inc ${_pango_lib} INTERFACE_INCLUDE_DIRECTORIES)
                if(_inc)
                    target_include_directories(${PROJECT_NAME} PRIVATE ${_inc})
                endif()
            endif()
        endforeach()
    endif()
else()
    # Fallback for older Pangolin installations
    target_include_directories(${PROJECT_NAME} PRIVATE ${Pangolin_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Pangolin_LIBRARIES})
endif()

# Link against OpenCV
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})

# --- realsense_app: RealSense left IR, right IR, depth in three viewports ---
find_package(realsense2 REQUIRED)

add_executable(realsense_app
    apps/realsense_app.cpp
    src/viewportal_display.cpp
    src/viewportal_params.cpp
    src/viewport_factory.cpp
    src/viewport_rgb8.cpp
    src/viewport_g8.cpp
    src/viewport_colored_depth.cpp
    src/viewport_reconstruction.cpp
    src/viewport_plot.cpp
)

target_include_directories(realsense_app PRIVATE
    include
    ${OpenCV_INCLUDE_DIRS}
    ${REALSENSE2_INCLUDE_DIR}
)

if(TARGET pangolin)
    target_link_libraries(realsense_app PRIVATE pangolin)
    get_target_property(PANGOLIN_INC pangolin INTERFACE_INCLUDE_DIRECTORIES)
    if(PANGOLIN_INC)
        target_include_directories(realsense_app PRIVATE ${PANGOLIN_INC})
    endif()
elseif(TARGET pango_display)
    if(Pangolin_LIBRARIES)
        target_link_libraries(realsense_app PRIVATE ${Pangolin_LIBRARIES})
        foreach(_pango_lib ${Pangolin_LIBRARIES})
            if(TARGET ${_pango_lib})
                get_target_property(_inc ${_pango_lib} INTERFACE_INCLUDE_DIRECTORIES)
                if(_inc)
                    target_include_directories(realsense_app PRIVATE ${_inc})
                endif()
            endif()
        endforeach()
    else()
        target_link_libraries(realsense_app PRIVATE pango_display pango_opengl pango_core pango_vars pango_image pango_plot pango_windowing)
        foreach(_pango_lib pango_display pango_opengl pango_core pango_vars pango_image pango_plot pango_windowing)
            if(TARGET ${_pango_lib})
                get_target_property(_inc ${_pango_lib} INTERFACE_INCLUDE_DIRECTORIES)
                if(_inc)
                    target_include_directories(realsense_app PRIVATE ${_inc})
                endif()
            endif()
        endforeach()
    endif()
else()
    target_include_directories(realsense_app PRIVATE ${Pangolin_INCLUDE_DIRS})
    target_link_libraries(realsense_app PRIVATE ${Pangolin_LIBRARIES})
endif()

target_link_libraries(realsense_app PRIVATE ${OpenCV_LIBS} realsense2::realsense2)
