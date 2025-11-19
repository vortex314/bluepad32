# glob(srcs "src/*.cpp" )

FILE(GLOB_RECURSE srcs ${CMAKE_SOURCE_DIR}/main/*.*)

set(requires "bluepad32" "btstack" )

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")
