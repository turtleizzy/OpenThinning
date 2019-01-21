cmake_minimum_required(VERSION 2.8)

if(POLICY CMP0020)
	cmake_policy(SET CMP0020 NEW)
endif()

project(OpenThinning)

option(WITH_ITK "Build ITK example" ON)
if(WITH_ITK)
	find_package(ITK REQUIRED)
	include(${ITK_USE_FILE})

	add_executable(OpenThinning_ITK Source/main_itk.cpp)
	target_link_libraries(OpenThinning_ITK ${ITK_LIBRARIES})
endif()
