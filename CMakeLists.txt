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
        src/Project.cpp
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

# Copy assets (will be populated in later phases)
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET DeepDAW POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:DeepDAW>/assets")
endif()