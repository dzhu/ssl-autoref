cmake_minimum_required (VERSION 2.8)
project (autoref)

#### miscellaneous compile settings
set (CMAKE_CXX_FLAGS "-std=c++11 -ggdb -Werror")
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set (CMAKE_EXPORT_COMPILE_COMMANDS on)
file (MAKE_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})

include_directories (shared)

#### compile protobufs
set (PROTO_IN_PATH ${PROJECT_SOURCE_DIR}/proto)
set (PROTO_OUT_PATH ${PROJECT_BINARY_DIR}/proto)

file (MAKE_DIRECTORY ${PROTO_OUT_PATH})
include_directories (${PROTO_OUT_PATH})

set (PROTO_FILES
  messages_robocup_ssl_detection
  messages_robocup_ssl_geometry
  messages_robocup_ssl_wrapper
  referee
  rcon
  )

set (CC_PROTO)
set (H_PROTO)

foreach (file ${PROTO_FILES})
  set (cc_file ${file}.pb.cc)
  set (h_file ${file}.pb.h)
  set (proto_file ${file}.proto)

  add_custom_command (OUTPUT ${PROTO_OUT_PATH}/${cc_file} ${PROTO_OUT_PATH}/${h_file}
    COMMAND protoc --cpp_out=${PROTO_OUT_PATH} --proto_path=${PROTO_IN_PATH} ${PROTO_IN_PATH}/${proto_file}
    DEPENDS ${PROTO_IN_PATH}/${proto_file}
    VERBATIM
    )

  list (APPEND CC_PROTO ${PROTO_OUT_PATH}/${cc_file})
  list (APPEND H_PROTO ${PROTO_OUT_PATH}/${h_file})
endforeach ()

add_custom_target (GenerateProto
  DEPENDS ${CC_PROTO} ${H_PROTO}
  VERBATIM
  )

add_library (shared_protobuf ${CC_PROTO})
add_dependencies (shared_protobuf GenerateProto)
target_link_libraries (shared_protobuf protobuf)

#### link main executable
add_executable (autoref
  autoref.cc
  autoref_main.cc
  base_ref.cc
  eval_ref.cc
  events.cc
  rconclient.cc
  shared/tracker.cc
  shared/udp.cc
  shared/util.cc
  touches.cc
  )
target_link_libraries (autoref shared_protobuf)
