

list(APPEND EXTRA_COMPONENT_DIRS icm20948)

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
    set(INC_FILES
    ".")
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
    set(INC_FILES
    ".")
    # message(SEND_ERROR "DECPATH: ${DECPATH}")
    # message(SEND_ERROR "Source files: ${SRC_FILES}")
elseif(CMAKE_MOTOR_CONTROLLER)
set(SRC_FILES
    "motorConfig/main.c"
    "pecan/common.cpp"
    "pecan/espSpecific.c"
    "espMutexes/mutex_declarations.c"
)
set(INC_FILES
".")
elseif(CMAKE_IMU_BASIC)
set(SRC_FILES
    "IMU_Basic/main.c"
    "icm20948/icm20948.c"
    "espMutexes/mutex_declarations.c"
)
set(INC_FILES
".")
elseif(CMAKE_IMU_COMPONENT_FILTERS)
set(SRC_FILES
    "IMU_Component_Filters/main.cpp")
    # "ESP_IMU_Sample/sample.cpp")
    set(INC_FILES
        "."
    )
#     append(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/components)
# set(COMPONENTS icm20948)
else()  #assumed "CMAKE_BASIC_NODE" configuration, just using one path
    set(SRC_FILES
        "${CMAKE_SRC_FILE}"
    )   
    set(INC_FILES
        ".")
        
endif()


idf_component_register(SRCS ${SRC_FILES} INCLUDE_DIRS ${INC_FILES})
# idf_component_register(SRCS ${SRC_FILES} INCLUDE_DIRS ".")

# idf_component_register(SRCS "pecan/espSpecific.c" "pecan/common.cpp" "vitalsNode/mutex_declarations.c" "vitalsNode/vitalsHelper/vitalsHelper.c" "vitalsNode/vitalsHelper/vitalsStaticDec.c" "vitalsNode/main.c" 
#                     INCLUDE_DIRS ".")
                    # "pecan" 
                    # "vitalsNode"
                    # "vitalsNode/vitalsHelper")

# idf_component_register(SRCS "vitalsHelper/vitalsStaticDec.c" "vitalsHelper/vitalsHelper.c" "pecan/common.c" "pecan/espSpecific.c" "main.c" "mutex_declarations.c"
#                     INCLUDE_DIRS ".")
