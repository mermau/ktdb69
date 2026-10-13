cmake_minimum_required(VERSION 3.22)

# Universal binary (Apple Silicon + Intel) su macOS
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(DB69 VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: usa una copia locale se indicata (-DJUCE_DIR=...), altrimenti la scarica
set(JUCE_DIR "" CACHE PATH "Cartella di JUCE (opzionale)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(DB69
    COMPANY_NAME "Mercuri Audio"
    BUNDLE_ID com.mercuriaudio.db69
    PLUGIN_MANUFACTURER_CODE Mrcu
    PLUGIN_CODE Db69
    PRODUCT_NAME "DB-69 Dub Filter"
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE          # riceve i CC dalla tastiera controller
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect   # AU standard ("aufx"): negli slot Audio FX di Logic
    VST3_CATEGORIES Fx Filter
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "DB-69 usa l'ingresso della scheda audio per elaborare il segnale."
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(DB69 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(DB69 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(DB69
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Test del modello DSP (senza JUCE)
option(DB69_BUILD_TESTS "Compila il test del circuito" ON)
if(DB69_BUILD_TESTS)
    add_executable(network_test tests/network_test.cpp)
    target_include_directories(network_test PRIVATE Source)
endif()

# Strumento di sviluppo: salva un'immagine dell'interfaccia (Linux/macOS)
option(DB69_BUILD_SNAPSHOT "Compila gli strumenti di sviluppo (screenshot, test del processore)" OFF)
if(DB69_BUILD_SNAPSHOT)
    juce_add_console_app(gui_snapshot PRODUCT_NAME "gui_snapshot")
    target_sources(gui_snapshot PRIVATE tests/gui_snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(gui_snapshot PRIVATE Source)
    target_compile_definitions(gui_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JucePlugin_Name="DB-69 Dub Filter" JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(gui_snapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)

    juce_add_console_app(processor_test PRODUCT_NAME "processor_test")
    target_sources(processor_test PRIVATE tests/processor_test.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(processor_test PRIVATE Source)
    target_compile_definitions(processor_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JucePlugin_Name="DB-69 Dub Filter" JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(processor_test PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
