# Generated from sharedimage.pro.

#####################################################################
## tst_sharedimage Test:
#####################################################################

# Collect test data

file(GLOB test_data_glob
    LIST_DIRECTORIES true
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "data/*")
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_sharedimage
    GUI
    SOURCES
        tst_sharedimage.cpp
    LIBRARIES
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Quick
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:sharedimage.pro:<TRUE>:
# CONFIG = "testcase" "-app_bundle"
# OTHER_FILES = "data/yellow.png"
