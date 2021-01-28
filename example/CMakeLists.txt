add_executable(example example.cc)
target_include_directories(example PUBLIC ${CMAKE_SOURCE_DIR}/include )
target_link_libraries(example PRIVATE Threads::Threads SQLite::SQLite3)
