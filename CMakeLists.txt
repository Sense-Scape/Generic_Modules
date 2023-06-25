# CMake instructions to make the static lib

# Min CMake version
cmake_minimum_required(VERSION 3.20)

set(SOURCE_DIR "source")
set(INCLUDE_DIR "include")

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