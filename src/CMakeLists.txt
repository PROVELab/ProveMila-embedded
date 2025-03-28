# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

#FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
#set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/vitalsNode)
if("VITALS_BUILD" IN_LIST PLATFORMIO_BUILD_FLAGS)       #VITALS_BUILD
    set(SRC_FILES 
        "vitalsHelper/vitalsStaticDec.c"
        "vitalsHelper/vitalsHelper.c"
        "pecan/common.c"
        "pecan/espSpecific.c"
        "main.c"
        "mutex_declarations.c"
    )
else("SENSOR_BUILD" IN_LIST PLATFORMIO_BUILD_FLAGS)   #SENSOR_BUILD. It is further expectected to have SENSORSPECIFIC be defined
    set(SRC_FILES main/env2_main.c)
endif()
idf_component_register(SRCS ${SRC_FILES} INCLUDE_DIRS ".")

# idf_component_register(SRCS "pecan/espSpecific.c" "pecan/common.cpp" "vitalsNode/mutex_declarations.c" "vitalsNode/vitalsHelper/vitalsHelper.c" "vitalsNode/vitalsHelper/vitalsStaticDec.c" "vitalsNode/main.c" 
#                     INCLUDE_DIRS ".")
                    # "pecan" 
                    # "vitalsNode"
                    # "vitalsNode/vitalsHelper")

# idf_component_register(SRCS "vitalsHelper/vitalsStaticDec.c" "vitalsHelper/vitalsHelper.c" "pecan/common.c" "pecan/espSpecific.c" "main.c" "mutex_declarations.c"
#                     INCLUDE_DIRS ".")
