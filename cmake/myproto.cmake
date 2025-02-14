find_package(Protobuf CONFIG REQUIRED)

add_library(myproto STATIC)

protobuf_generate(
  LANGUAGE cpp
  TARGET myproto
  PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/go/grpc_server/gen/libcore.proto
)

target_link_libraries(myproto
  PUBLIC protobuf::libprotobuf
)

target_include_directories(myproto
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
)
