set(EDGE_IMPULSE_DIR "${CMAKE_CURRENT_LIST_DIR}/../coolbeebox-cpp-mcu-v1-impulse-#1")
set(EI_SDK_DIR "${EDGE_IMPULSE_DIR}/edge-impulse-sdk")

file(GLOB_RECURSE EI_CPP_SOURCES
    "${EI_SDK_DIR}/*.cpp"
    "${EI_SDK_DIR}/*.cc"
    "${EI_SDK_DIR}/*.c"
)
file(GLOB_RECURSE EI_MODEL_SOURCES
    "${EDGE_IMPULSE_DIR}/tflite-model/*.cpp"
)
file(GLOB_RECURSE EI_ESP_NN_ASM_SOURCES
    "${EI_SDK_DIR}/porting/espressif/ESP-NN/*.s"
    "${EI_SDK_DIR}/porting/espressif/ESP-NN/*.S"
)
file(GLOB_RECURSE EI_ESP_DSP_ASM_SOURCES
    "${EI_SDK_DIR}/porting/espressif/esp-dsp/modules/fft/*.s"
    "${EI_SDK_DIR}/porting/espressif/esp-dsp/modules/fft/*.S"
)

foreach(source_file IN LISTS EI_CPP_SOURCES)
    if(source_file MATCHES "/CMSIS/")
        list(REMOVE_ITEM EI_CPP_SOURCES "${source_file}")
    endif()
endforeach()

list(APPEND EI_CPP_SOURCES
    "${EI_SDK_DIR}/CMSIS/DSP/Source/StatisticsFunctions/arm_rms_f32.c"
)

idf_component_register(
    SRCS
        "main.cpp"
        "wifi_app.c"
        "edgeai_mqtt_app.c"
        ${EI_CPP_SOURCES}
        ${EI_MODEL_SOURCES}
        ${EI_ESP_NN_ASM_SOURCES}
        ${EI_ESP_DSP_ASM_SOURCES}
    INCLUDE_DIRS
        "."
        "${EDGE_IMPULSE_DIR}"
        "${EI_SDK_DIR}"
        "${EI_SDK_DIR}/porting"
        "${EI_SDK_DIR}/porting/espressif/esp-dsp/modules/common/include"
        "${EI_SDK_DIR}/porting/espressif/esp-dsp/modules/fft/include"
        "${EI_SDK_DIR}/porting/espressif/ESP-NN"
        "${EI_SDK_DIR}/porting/espressif/ESP-NN/include"
    REQUIRES
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        mqtt
        esp_timer
        esp_driver_i2s
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    EI_CLASSIFIER_USE_FULL_TFLITE=0
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=missing-field-initializers
    -Wno-error=maybe-uninitialized
    -Wno-error=unused-variable
    -Wno-error=unused-function
)
