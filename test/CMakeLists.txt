add_executable(Reflection_Unittest Reflection_Unittest.cc)
target_include_directories(Reflection_Unittest PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(Reflection_Unittest ${CONAN_LIBS})
add_test(NAME Reflection_Unittest COMMAND Reflection_Unittest)

add_executable(Types_Unittest Types_Unittest.cc)
target_include_directories(Types_Unittest PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(Types_Unittest ${CONAN_LIBS})
add_test(NAME Types_Unittest COMMAND Types_Unittest)
