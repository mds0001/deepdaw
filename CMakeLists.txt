cmake_minimum_required(VERSION 3.25)

project(DeepDAW VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# JUCE configuration - fetch latest stable 8.x (trigger rebuild)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Main executable
juce_add_gui_app(DeepDAW
    PRODUCT_NAME "DeepDAW"
    COMPANY_NAME "DeepDAW"
    VERSION "0.1.0"
    BUNDLE_ID "com.deepdaw.app"
    # ICON_BIG will be added in Phase 1 with proper artwork
)

target_sources(DeepDAW
    PRIVATE
        src/Main.cpp
        src/MainComponent.cpp
        src/TransportComponent.cpp
        src/LookAndFeel.cpp
        src/TrackListComponent.cpp
        src/TrackHeaderComponent.cpp
        src/TimelineComponent.cpp
        src/TimelineRulerComponent.cpp
        src/ClipComponent.cpp
        src/Project.cpp
        src/ChannelStripComponent.cpp
        src/MixerComponent.cpp
)

target_include_directories(DeepDAW PRIVATE src)

target_link_libraries(DeepDAW
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Enable VST3 hosting support for later phases
juce_generate_juce_header(DeepDAW)

# Build id: regenerate BuildInfo.h (git short SHA) on every build so the app
# can show which commit it was built from. Seed an initial header at configure
# time so the include resolves before the custom target first runs.
set(DEEPDAW_GEN_DIR "${CMAKE_BINARY_DIR}/generated")
set(DEEPDAW_BUILD_INFO "${DEEPDAW_GEN_DIR}/BuildInfo.h")
file(MAKE_DIRECTORY "${DEEPDAW_GEN_DIR}")
if(NOT EXISTS "${DEEPDAW_BUILD_INFO}")
    file(WRITE "${DEEPDAW_BUILD_INFO}" "#pragma once\n#define DEEPDAW_BUILD_ID \"dev\"\n")
endif()
add_custom_target(deepdaw_build_info
    BYPRODUCTS "${DEEPDAW_BUILD_INFO}"
    COMMAND ${CMAKE_COMMAND}
        -DSRC=${CMAKE_SOURCE_DIR}
        -DOUTPUT=${DEEPDAW_BUILD_INFO}
        -P ${CMAKE_SOURCE_DIR}/cmake/GenBuildInfo.cmake
    COMMENT "Updating build id")
add_dependencies(DeepDAW deepdaw_build_info)
target_include_directories(DeepDAW PRIVATE "${DEEPDAW_GEN_DIR}")

# Copy assets (will be populated in later phases)
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET DeepDAW POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:DeepDAW>/assets")
endif()

# Optional: after each build, drop the executable into a fixed local folder so
# there is always one stable path to the latest build. Enable per-machine with
#   -DDEEPDAW_DEPLOY_DIR=C:/path/to/folder
# Left unset in CI, so it is a no-op there (no hard-coded personal paths).
if(DEFINED DEEPDAW_DEPLOY_DIR)
    add_custom_command(TARGET DeepDAW POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEEPDAW_DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:DeepDAW>" "${DEEPDAW_DEPLOY_DIR}/DeepDAW.exe")
endif()