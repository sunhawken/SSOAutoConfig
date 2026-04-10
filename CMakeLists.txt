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
set(SKYRIM_FOLDER "" CACHE PATH "Skyrim install directory (e.g. G:/SteamLibrary/.../Skyrim Special Edition)")

if(SKYRIM_FOLDER)
    set(DLL_FOLDER "${SKYRIM_FOLDER}/Data/SKSE/Plugins")
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )
endif()
