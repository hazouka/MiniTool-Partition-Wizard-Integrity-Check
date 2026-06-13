cmake_minimum_required(VERSION 3.16)

project(WizardMfh)

set(CMAKE_PREFIX_PATH "D:/Qt/6.11.1/msvc2022_64")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(MfhGenerator src/Generator.cpp)
add_executable(MfhParser src/Parser.cpp)

target_link_libraries(MfhGenerator PRIVATE Qt6::Widgets)
target_link_libraries(MfhParser PRIVATE Qt6::Widgets)

set_target_properties(MfhGenerator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin/MfhGenerator/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/MfhGenerator/Release"
)

set_target_properties(MfhParser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin/MfhParser/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/MfhParser/Release"
)