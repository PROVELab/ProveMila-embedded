# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

#FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
#set(EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/vitalsNode)
# if("VITALS_ESP_BUILD" IN_LIST PLATFORMIO_BUILD_FLAGS)       #VITALS_BUILD

if(CMAKE_VITALS_ESP_BUILD)
    # message(SEND_ERROR "SRC_FILES for VITALS_BUILD: ${SRC_FILES}")

    set(SRC_FILES 
        "vitalsNode/vitalsHelper/vitalsStaticDec.c"
        "vitalsNode/vitalsHelper/vitalsHelper.c"
        "pecan/common.cpp"
        "pecan/espSpecific.c"
        "vitalsNode/main.c"
        "vitalsNode/mutex_declarations.c"
    )
    message(STATUS "SRC_FILES for VITALS_BUILD: ${SRC_FILES}")

# elseif("SENSOR_BUILD" IN_LIST PLATFORMIO_BUILD_FLAGS)  # SENSOR_BUILD
else()  #CMAKE_SENSOR_ESP_BUILD
    set(SRC_FILES
        "sensors/genericNodeNameESP/genericNodeNamestaticDec.cpp"
        "sensors/genericNodeNameESP/genericNodeNameESP_main.c"
        # "sensors/genericNodeNameESP/my"


        # "sensors/genericNodeNameESP/"
        "sensors/common/sensorHelper.cpp"
        "pecan/common.cpp"
        "pecan/espSpecific.c"
        "vitalsNode/mutex_declarations.c"
    )

    # Handle subtraction (exclude specific files like espSpecific.c)
    # list(REMOVE_ITEM SRC_FILTER_LIST "pecan/arduinoSpecific.c")
endif()
idf_component_register(SRCS ${SRC_FILES} INCLUDE_DIRS ".")

# idf_component_register(SRCS "pecan/espSpecific.c" "pecan/common.cpp" "vitalsNode/mutex_declarations.c" "vitalsNode/vitalsHelper/vitalsHelper.c" "vitalsNode/vitalsHelper/vitalsStaticDec.c" "vitalsNode/main.c" 
#                     INCLUDE_DIRS ".")
                    # "pecan" 
                    # "vitalsNode"
                    # "vitalsNode/vitalsHelper")

# idf_component_register(SRCS "vitalsHelper/vitalsStaticDec.c" "vitalsHelper/vitalsHelper.c" "pecan/common.c" "pecan/espSpecific.c" "main.c" "mutex_declarations.c"
#                     INCLUDE_DIRS ".")
