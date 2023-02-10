cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(PythonInterp REQUIRED)
project(controller)

add_custom_command(TARGET app
     PRE_BUILD
     COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/updatebuild.py" "${CMAKE_CURRENT_SOURCE_DIR}/include"
     COMMENT "This command will generate the build.h file"
     VERBATIM
)
zephyr_include_directories(src)
file (GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)
target_sources(app PRIVATE ${SRC_FILES})
zephyr_library_include_directories(.)
zephyr_library_include_directories(lib)
zephyr_library_include_directories(include)
