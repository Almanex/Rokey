cmake_minimum_required(VERSION 3.20)
project(Roke VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Windows definitions
add_definitions(-DUNICODE -D_UNICODE)

# Source files
set(SOURCES 
    src/main.cpp 
    src/resources.rc
)

add_executable(Roke WIN32 ${SOURCES})

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set_target_properties(Roke PROPERTIES LINK_FLAGS "-mwindows")
endif()

# Windows libraries
target_link_libraries(Roke PRIVATE user32 kernel32 shell32 ole32 advapi32 dwmapi uxtheme)

# Include resource headers
target_include_directories(Roke PRIVATE ${CMAKE_SOURCE_DIR}/src)
