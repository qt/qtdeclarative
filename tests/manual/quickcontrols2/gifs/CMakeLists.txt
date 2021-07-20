# Generated from gifs.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_gifs LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_gifs Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_manual_test(tst_gifs
    SOURCES
        capturedevent.cpp capturedevent.h
        eventcapturer.cpp eventcapturer.h
        gifrecorder.cpp gifrecorder.h
        tst_gifs.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:gifs.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
