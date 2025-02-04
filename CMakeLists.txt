# Min CMake version
cmake_minimum_required(VERSION 3.18)
project(GenericModulesProject CXX)

# Set directory vars
set(SOURCE_DIR "source")
set(INCLUDE_DIR "include")
set(TEST_DIR "tests")

message(STATUS "Building for Unix")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)

# Main executable
file(GLOB SOURCES "${SOURCE_DIR}/*.cpp")
add_library(GenericModuleLib
    ${SOURCES}
)


if (NOT TARGET BaseModuleLib)
    add_subdirectory(components/Base_Module)
endif()

find_library(GPS_LIBRARY gps REQUIRED)
if(GPS_LIBRARY)
  message(STATUS "Found GPS library: ${GPS_LIBRARY}")
else()
  message(FATAL_ERROR "GPS library not found!")
endif()

add_subdirectory(components/kissfft)

# Include directories for main code and tests
target_include_directories(GenericModuleLib
    PRIVATE ${INCLUDE_DIR}
    PRIVATE ${INCLUDE_TESTS_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/include     
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include/nlohmann
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/plog/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/kissfft	
)

# Find Google Test package
find_package(GTest REQUIRED)
# Main executable

file(GLOB TESTS "${TEST_DIR}/*.cpp")
message(STATUS ${TESTS})
add_executable(GenericTests ${TESTS})

target_include_directories(GenericTests
    PRIVATE ${INCLUDE_DIR}
    PRIVATE ${INCLUDE_TESTS_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/include     
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include/nlohmann
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/plog/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/kissfft	
)

target_link_libraries(GenericTests PRIVATE GenericModuleLib BaseModuleLib ChunkTypesLib GTest::GTest GTest::Main gps)
