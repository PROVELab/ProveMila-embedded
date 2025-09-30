
# list(APPEND EXTRA_COMPONENT_DIRS icm20948)

if(CMAKE_VITALS_ESP_BUILD)

    set(SRC_FILES 
        "vitalsNode/vitalsHelper/vitalsStaticDec.c"
        "vitalsNode/vitalsHelper/vitalsHelper.c"
        "pecan/common.cpp"
        "pecan/espSpecific.c"
        "vitalsNode/main.c"
        "espBase/debug_esp.c"
        "vitalsNode/vitalsData.c"
        "vitalsNode/vitalsHB.c"
    )

elseif(CMAKE_SENSOR_ESP_BUILD)  #CMAKE_SENSOR_ESP_BUILD <- Assumed

    set(SRC_FILES
        "sensors/${SENS_DIR}/main.c"
        "sensors/${SENS_DIR}/staticDec.cpp"
        "sensors/common/sensorHelper.cpp"
        "pecan/common.cpp"
        "pecan/espSpecific.c"
        "pecan/vitalsInit.c"
        "espBase/debug_esp.c"
    )

elseif(CMAKE_MOTOR_CONTROLLER)
set(SRC_FILES
    "motorConfig/main.c"
    "pecan/common.cpp"
    "pecan/espSpecific.c"
    "espBase/debug_esp.c"
)
else()  #assumed "CMAKE_BASIC_NODE" configuration, just using one path
    set(SRC_FILES
        "${CMAKE_SRC_FILE}"
    )   
endif()

idf_component_register( SRCS ${SRC_FILES}
                        INCLUDE_DIRS ".")
