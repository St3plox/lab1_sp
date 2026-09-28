cmake_minimum_required(VERSION 3.16)
project(masked_line_edit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

add_library(masked_line_edit maskedlineedit.cpp maskedlineedit.h)
target_link_libraries(masked_line_edit PUBLIC Qt6::Widgets)
target_include_directories(masked_line_edit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mask_demo main.cpp)
target_link_libraries(mask_demo PRIVATE masked_line_edit)

enable_testing()
add_executable(mask_tests tests.cpp)
target_link_libraries(mask_tests PRIVATE masked_line_edit Qt6::Test)
add_test(NAME mask_tests COMMAND mask_tests)
set_tests_properties(mask_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
