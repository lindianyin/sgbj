
cmake_minimum_required(VERSION 1.0)

project(cardblade)
set(VERSION 1.0)
set(EXECUTABLE_OUTPUT_PATH ${cardblade_SOURCE_DIR}/install)

include_directories(/usr/local/include /usr/include/mysql /usr/local/mysql/include /usr/local/include/boost ./mysqlwrapped-1.6.1)
link_directories(/usr/local/lib /usr/lib64/mysql  /usr/local/mysql/lib/mysql)
add_definitions(-DQQ_PLAT -DONE_CHARACTOR -DUSE_TGW -DDEBUG_PER -DCAMP_RACE_TWO_QUEUE)
set(CMAKE_EXE_LINKER_FLAGS "-Wall -g -O1 -MD")

execute_process(COMMAND date +%Y%m%d OUTPUT_VARIABLE DATE)

aux_source_directory(./mysqlwrapped-1.6.1 src_mysql)
aux_source_directory(./ src_cardblade)
add_executable(cardblade ${src_cardblade} ${src_mysql})

target_link_libraries(cardblade libboost_thread.a libboost_system.a libboost_date_time.a libboost_regex.a)
target_link_libraries(cardblade libmysqlclient_r.a -lpthread -lz)

add_executable(card_event tools/add_event.cpp ${src_mysql})
target_link_libraries(card_event libmysqlclient_r.a -lz -lpthread)

