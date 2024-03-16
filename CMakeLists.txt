cmake_minimum_required (VERSION 3.2)

project (Contact)

include_directories (Server ConnectionPool Session CoreAlgorithm cppjieba  cppjieba/limonp)
include_directories(/usr/include/mysql)

aux_source_directory (Server Server_SRC)
aux_source_directory (Session Session_SRC)
aux_source_directory (MatchScheduler MatchScheduler_SRC)
aux_source_directory (ConnectionPool ConnectionPool_SRC)
aux_source_directory (BlockDataStruct BlockDataStruct_SRC)

add_compile_options(-std=c++20 -w) 

add_executable (Contact_Server main.cpp ${Server_SRC} ${ConnectionPool_SRC} ${Session_SRC} ${MatchScheduler_SRC} ${BlockDataStruct_SRC})
target_link_libraries(Contact_Server mysqlclient)