# CMake instructions to make the static lib

# Min CMake version
cmake_minimum_required(VERSION 3.20)

idf_component_register(SRCS "RouterModule.cpp" 
							"SessionProcModule.cpp"
							"WavAccumulator.cpp"
							"SimulatorModule.cpp"
							"WatchdogModule.cpp"
						INCLUDE_DIRS "./include"
						REQUIRES Chunk_Types Base_Module
						)
