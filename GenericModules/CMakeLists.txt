# CMake instructions to make the static lib

# Min CMake version
cmake_minimum_required(VERSION 3.24)

# Project name and langues (C, C++, Assembly)
project(SC_Generic_Module_Types C CXX ASM)

# 			-L-
# Adding Chunk Types libs to modules
# 			---
# Adding Chunk Types to modules
find_library(SC_Chunk_Types
			PUBLIC
			$ENV{SC_LIB_DIR}/Lib_Chunk_Types/CPP_Chunk_Types/build
			)
include_directories(SC_Chunk_Types
				INTERFACE
				$ENV{SC_LIB_DIR}/Lib_Chunk_Types/CPP_Chunk_Types
				)


# 			-L-
# Adding Base Module libs to Generic Modules
# 			---
find_library(SC_Base_Module_Types
			PUBLIC
			$ENV{SC_LIB_DIR}/Lib_Base_Module/BaseModule/build
			)
include_directories(SC_Base_Module_Types
				INTERFACE
				$ENV{SC_LIB_DIR}/Lib_Base_Module/BaseModule
				)

# What to add to library
add_library(SC_Generic_Module_Types 
			STATIC
			RouterModule.h
			RouterModule.cpp
			SessionProcModule.h
			SessionProcModule.cpp
			WavAccumulator.h
			WavAccumulator.cpp
			WAVWriterModule.h
			WAVWriterModule.cpp
			)

# Adding sources to library
target_sources(SC_Generic_Module_Types
				PUBLIC 
				RouterModule.h
				RouterModule.cpp
				SessionProcModule.h
				SessionProcModule.cpp
				WavAccumulator.h
				WavAccumulator.cpp
				WAVWriterModule.h
				WAVWriterModule.cpp
				)