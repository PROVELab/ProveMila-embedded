# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

# if(CMAKE_SENSOR_ESP_BUILD)#DEFINED SENSOR_ESP_BUILD AND 
#     # message(SEND_ERROR "Feature enabled")
# else()
#     # message(SEND_ERROR "no feature enabled")
# endif()

# if(CMAKE_VITALS_ESP_BUILD)
#     message(SEND_ERROR "Feature enabled2")
# else()
#     message(SEND_ERROR "no feature enabled2")
# endif()
# if(DECPATH STREQUAL "src/sensors/genericNodeNameESP/genericNodeNamestaticDec.cpp")
#     message(SEND_ERROR "is defined")
# else()
#     message(SEND_ERROR "is not defined")
# endif()

if(CMAKE_VITALS_ESP_BUILD)
    # message(SEND_ERROR "SRC_FILES for VITALS_BUILD: ${SRC_FILES}")
    # set(Path)
    set(SRC_FILES 
        "vitalsNode/vitalsHelper/vitalsStaticDec.c"
        "vitalsNode/vitalsHelper/vitalsHelper.c"
        "pecan/common.cpp"
        "pecan/espSpecific.c"
        "vitalsNode/main.c"
        "vitalsNode/mutex_declarations.c"
    )
    # message(SEND_ERROR "SRC_FILES for VITALS_BUILD: ${SRC_FILES}")

elseif(CMAKE_SENSOR_ESP_BUILD)  #CMAKE_SENSOR_ESP_BUILD <- Assumed
    # message(SEND_ERROR "Path is set to: ${DECPath}")

    set(SRC_FILES
        "sensors/${SENS_DIR}/main.c"
        "sensors/${SENS_DIR}/staticDec.cpp"
        
        "sensors/common/sensorHelper.cpp"
        "pecan/common.cpp"
        "pecan/espSpecific.c"
        "espMutexes/mutex_declarations.c"
    )
    # message(SEND_ERROR "DECPATH: ${DECPATH}")
    # message(SEND_ERROR "Source files: ${SRC_FILES}")
elseif(CMAKE_MCU_ESP_BUILD)
    # message(SEND_ERROR "CMAKE_MCU_ESP_BUILD is set, using MCU ESP build configuration")
    set(SRC_FILES
        "mcu/main.c"
        "mcu/vsr.c"
        "mcu/motor_h300/h300.c"
        "mcu/motor_h300/motor_send.c"
        "pecan/common.cpp"
        "pecan/espSpecific.c"
        "espMutexes/mutex_declarations.c"
    )
else()  #assumed "CMAKE_BASIC_NODE" configuration, just using one path
    set(SRC_FILES
        "${CMAKE_SRC_FILE}"
    )   
endif()
idf_component_register(SRCS ${SRC_FILES} INCLUDE_DIRS ".")


# idf_component_register(SRCS "pecan/espSpecific.c" "pecan/common.cpp" "vitalsNode/mutex_declarations.c" "vitalsNode/vitalsHelper/vitalsHelper.c" "vitalsNode/vitalsHelper/vitalsStaticDec.c" "vitalsNode/main.c" 
#                     INCLUDE_DIRS ".")
                    # "pecan" 
                    # "vitalsNode"
                    # "vitalsNode/vitalsHelper")

# idf_component_register(SRCS "vitalsHelper/vitalsStaticDec.c" "vitalsHelper/vitalsHelper.c" "pecan/common.c" "pecan/espSpecific.c" "main.c" "mutex_declarations.c"
#                     INCLUDE_DIRS ".")
