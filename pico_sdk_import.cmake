# Minimal project-local import shim for the official Raspberry Pi Pico SDK.
# The SDK remains an external dependency; no SDK code is vendored here.
if (NOT PICO_SDK_PATH)
    if (DEFINED ENV{PICO_SDK_PATH})
        set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "Path to the Raspberry Pi Pico SDK")
    else()
        message(FATAL_ERROR "PICO_SDK_PATH is not set. Set it to your local pico-sdk checkout.")
    endif()
endif()

if (NOT EXISTS "${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
    message(FATAL_ERROR "Cannot find ${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
endif()
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
