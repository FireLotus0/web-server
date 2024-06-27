cmake_minimum_required(VERSION 3.2)

PROJECT(HSQ)


#设置编译器参数
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER g++)


SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS}  -O0 -Wall -g2 -ggdb -Wno-builtin-macro-redefined")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS}  -O3 -Wall -Wno-builtin-macro-redefined")

#sqlite static lib path
SET(SQLITE_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/lib/libsqlite3.a)
#sqlite include file dir path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

set(CURPATH ${PROJECT_SOURCE_DIR})
include(${CURPATH}/util_cmake/util.cmake)

#头文件包含目录
include_directories(${CURPATH}/src/)
include_directories(${CURPATH}/src/http/)
include_directories(${CURPATH}/src/thirdparty/include/rapidjson/)
include_directories(${CURPATH}/src/db/)
include_directories(${CURPATH}/server/)

#源文件代码SRC
aux_source_directory(${CURPATH}/src/ SRC)
aux_source_directory(${CURPATH}/src/http/ SRC)
aux_source_directory(${CURPATH}/src/db/ SRC)

#链接库
link_libraries(pthread dl)

#设置程序生成目录
set(EXECUTABLE_OUTPUT_PATH  ${CURPATH}/tests/)

#add_executable(test_hook ${CURPATH}/tests/test_hook.cc ${SRC})
#redefine_file_macro(test_hook)

#add_executable(test_bytearray ${CURPATH}/tests/test_bytearray.cc ${SRC})
#redefine_file_macro(test_bytearray)


#add_executable(test_addr ${CURPATH}/tests/test_addr.cc ${SRC})
#redefine_file_macro(test_addr)

#add_executable(test_http ${CURPATH}/tests/test_http.cc ${SRC})
#redefine_file_macro(test_http)

#add_executable(test_socket ${CURPATH}/tests/test_socket.cc ${SRC})
#redefine_file_macro(test_socket)

#add_executable(test_iomanager ${CURPATH}/tests/test_iomanager.cc ${SRC})
#redefine_file_macro(test_iomanager)

#add_executable(test_scheduler ${CURPATH}/tests/test_scheduler.cc ${SRC})
#redefine_file_macro(test_scheduler)


#add_executable(test_tcpserver ${CURPATH}/tests/test_tcpserver.cc ${SRC})
#redefine_file_macro(test_tcpserver)

#add_executable(test_serverlet ${CURPATH}/tests/test_serverlet.cc ${SRC})
#redefine_file_macro(test_serverlet)
#target_link_libraries(test_serverlet ${SQLITE_LIBRARY})


#add_executable(test_db ${CURPATH}/tests/test_db.cc ${SRC})
#redefine_file_macro(test_db)
#target_link_libraries(test_db ${SQLITE_LIBRARY})

#add_executable(test_json ${CURPATH}/tests/test_json.cc ${SRC})
#redefine_file_macro(test_json)


add_executable(configure ${CURPATH}/server/configure.cc ${SRC})
redefine_file_macro(configure)
target_link_libraries(configure ${SQLITE_LIBRARY})
set_target_properties(configure PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CURPATH}/server/"
)

add_executable(server ${CURPATH}/server/server.cc ${CURPATH}/server/dao.cc  ${SRC})
redefine_file_macro(server)
target_link_libraries(server ${SQLITE_LIBRARY})
set_target_properties(server PROPERTIES
 RUNTIME_OUTPUT_DIRECTORY "${CURPATH}/server/"
 )





