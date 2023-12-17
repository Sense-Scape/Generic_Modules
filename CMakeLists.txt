# Min CMake version
cmake_minimum_required(VERSION 3.18)
project(BaseModuleProject CXX)

# Set directory vars
set(SOURCE_DIR "source")
set(INCLUDE_DIR "include")
set(SOURCE_TESTS_DIR "source_tests")
set(INCLUDE_TESTS_DIR "include_tests")

if(DEFINED ENV{IDF_PATH}) # this is normally only set when running in an esp idf terminal
    message(STATUS "Building for ESP32")

    idf_component_register(
	SRCS
		${SOURCE_DIR}/RouterModule.cpp
		${SOURCE_DIR}/SessionProcModule.cpp
		${SOURCE_DIR}/WavAccumulator.cpp
		${SOURCE_DIR}/SimulatorModule.cpp
		${SOURCE_DIR}/WatchdogModule.cpp
	INCLUDE_DIRS
		${INCLUDE_DIR}
	REQUIRES 
		Chunk_Types 
		Base_Module
)

else()
    message(STATUS "Building for Unix")

    # Set C++ standard
    set(CMAKE_CXX_STANDARD 20)

    # Main executable
    file(GLOB SOURCES "${SOURCE_DIR}/*.cpp")
    add_library(GenericModuleLib
        ${SOURCES}
    )

    # Test executable
    add_executable(GenericModuleTests
        ${SOURCE_TESTS_DIR}/GenericModulesTests.cpp 
    )

    add_subdirectory(components/Base_Module)
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

    target_include_directories(GenericModuleTests
        PRIVATE ${INCLUDE_DIR}
        PRIVATE ${INCLUDE_TESTS_DIR}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/components/doctest/doctest
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/Chunk_Types/components/Nlohmann_JSON/include/nlohmann
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/Base_Module/components/plog/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/components/kissfft	
    )

    # Library A which need B shall be listed before B
    target_link_libraries(GenericModuleTests PRIVATE BaseModuleLib GenericModuleLib ChunkTypesLib kissfft::kissfft)

    # Enable testing
    enable_testing()

    # Add tests to CTest
    add_test(NAME GenericModuleTests COMMAND GenericModuleTests)

endif()
