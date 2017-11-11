# -* cmake -*-

find_package(FlatBuffers REQUIRED)

build_flatbuffers("apidef.fbs" "" "api" "" "." "" "")
