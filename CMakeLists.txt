
cmake_minimum_required(VERSION 2.8)

project(task)

add_executable(helloWorld test_comparator.cpp)

set_target_properties(
	helloWorld PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)

