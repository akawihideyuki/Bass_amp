cmake_minimum_required(VERSION 3.22)

project(BassAmp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

set(BASSAMP_CORE_SOURCES
    src/dsp/BassAmpProcessor.cpp
    src/dsp/BassAmpProcessor.h
    src/tuner/PitchDetector.cpp
    src/tuner/PitchDetector.h
    src/tuner/TunerEngine.cpp
    src/tuner/TunerEngine.h
    src/preset/PresetManager.cpp
    src/preset/PresetManager.h
    src/preset/PresetState.h
    src/audio/AudioSettings.cpp
    src/audio/AudioSettings.h
    src/diagnostics/Diagnostics.h
)

juce_add_gui_app(BassAmp
    PRODUCT_NAME "Bass_amp"
    COMPANY_NAME "akawihideyuki"
    BUNDLE_ID "com.akawihideyuki.bassamp"
    VERSION ${PROJECT_VERSION}
)

juce_generate_juce_header(BassAmp)

target_sources(BassAmp PRIVATE
    src/Main.cpp
    src/MainComponent.cpp
    src/MainComponent.h
    ${BASSAMP_CORE_SOURCES}
)

target_include_directories(BassAmp PRIVATE src)

target_compile_definitions(BassAmp PRIVATE
    JUCE_ASIO=1
    JUCE_ASIO_USE_EXTERNAL_SDK=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(BassAmp PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

enable_testing()

juce_add_console_app(BassAmpTests
    PRODUCT_NAME "BassAmpTests"
)

juce_generate_juce_header(BassAmpTests)

target_sources(BassAmpTests PRIVATE
    tests/Tests.cpp
    src/dsp/BassAmpProcessor.cpp
    src/dsp/BassAmpProcessor.h
    src/tuner/PitchDetector.cpp
    src/tuner/PitchDetector.h
    src/preset/PresetManager.cpp
    src/preset/PresetManager.h
    src/preset/PresetState.h
)

target_include_directories(BassAmpTests PRIVATE src)

target_compile_definitions(BassAmpTests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(BassAmpTests PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME BassAmpTests COMMAND BassAmpTests)
