cmake_minimum_required(VERSION 3.24)

project(
    SSSOAutoConfig
    VERSION 1.0.0
    LANGUAGES CXX
)

find_package(CommonLibSSE CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME}
    AUTHOR "Young Jonii"
    SOURCES
        src/main.cpp
        src/PCH.h
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)
target_include_directories(${PROJECT_NAME} PRIVATE src)

# --- auto-deploy DLL to SKSE/Plugins after build ---
# A single DLL works for Skyrim SE, AE, and VR thanks to CommonLibSSE-NG's
# multi-target support, so we can deploy to both an SSE install and a VR
# install in the same build.  Set either, neither, or both of these cache
# variables; a non-empty value enables the matching copy step.
set(SKYRIM_FOLDER   "" CACHE PATH "Skyrim Special Edition install directory")
set(SKYRIMVR_FOLDER "" CACHE PATH "Skyrim VR install directory")

function(_ssso_deploy_to GAME_FOLDER LABEL)
    if(GAME_FOLDER)
        set(DLL_FOLDER "${GAME_FOLDER}/Data/SKSE/Plugins")
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:${PROJECT_NAME}>"
                "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            COMMENT "Deploying ${PROJECT_NAME}.dll to ${LABEL}"
            VERBATIM
        )
    endif()
endfunction()

_ssso_deploy_to("${SKYRIM_FOLDER}"   "Skyrim SE")
_ssso_deploy_to("${SKYRIMVR_FOLDER}" "Skyrim VR")
