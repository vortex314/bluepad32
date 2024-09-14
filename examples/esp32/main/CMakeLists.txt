set(srcs "main.cpp" "esp_now_platform.cpp" "esp_gtw.cpp" "codec.cpp" "Log.cpp" "Sema.cpp" "Sys.cpp" )

set(requires "bluepad32" "btstack" )

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")
