cmake_minimum_required(VERSION 3.22)

project(BassAmp VERSION 0.1.0 LANGUAGES C CXX)

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

juce_add_gui_app(BassAmp
    PRODUCT_NAME "Bass_amp"
    COMPANY_NAME "akawihideyuki"
    BUNDLE_ID "com.akawihideyuki.bassamp"
)

juce_generate_juce_header(BassAmp)

target_sources(BassAmp PRIVATE
    src/Main.cpp
    src/MainComponent.cpp
    src/MainComponent.h
    src/dsp/BassAmpProcessor.cpp
    src/dsp/BassAmpProcessor.h
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
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
