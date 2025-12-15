
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
    # message(SEND_ERROR "SRC_FILES for VITALS_BUILD: ${SRC_FILES}")
endif()

if(CMAKE_SENSOR_ESP_BUILD)  #CMAKE_SENSOR_ESP_BUILD <- Assumed
    # message(SEND_ERROR "Path is set to: ${DECPath}")

    file(GLOB SENSOR_SRC_FILES
        "sensors/${SENS_DIR}/*.c"
        "sensors/${SENS_DIR}/*.cpp"
    )
    set(SRC_FILES
        # "sensors/${SENS_DIR}/main.c"
        "sensors/${SENS_DIR}/staticDec.cpp"
        
        "sensors/common/sensorHelper.cpp"
        "pecan/common.cpp"
        "pecan/espSpecific.c"
        "pecan/vitalsInit.c"
        # "espMutexes/mutex_declarations.c"
    )
    # message(SEND_ERROR "DECPATH: ${DECPATH}")
    # message(SEND_ERROR "Source files: ${SRC_FILES}")
endif()

if(CMAKE_MCU_ESP_BUILD)
    # message(SEND_ERROR "CMAKE_MCU_ESP_BUILD is set, using MCU ESP build configuration")
    # Recursively collect sources under mcu/ (makes stuff easier)
    file(GLOB_RECURSE MCU_SRC
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/espBase/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/mcu/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/mcu/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/mcu/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mcu/*.cxx"

        "${CMAKE_CURRENT_SOURCE_DIR}/sensors/common/sensorHelper.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/sensors/powerSensor/espPower.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/sensors/pedalSensor/*.c"
    )

    # If you also want to include other existing files outside mcu/, add them here:
    set(SRC_FILES
        ${MCU_SRC} ${SRC_FILES}
    )
endif()

# else()  #assumed "CMAKE_BASIC_NODE" configuration, just using one path
#     set(SRC_FILES
#         "${CMAKE_SRC_FILE}"
#     )   
# endif()
idf_component_register(SRCS ${SRC_FILES} INCLUDE_DIRS ".")
