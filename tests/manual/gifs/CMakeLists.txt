# Generated from gifs.pro.

#####################################################################
## tst_gifs Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_gifs
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
