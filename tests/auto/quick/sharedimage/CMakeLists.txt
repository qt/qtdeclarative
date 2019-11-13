# Generated from sharedimage.pro.

#####################################################################
## tst_sharedimage Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

add_qt_test(tst_sharedimage
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
# OTHER_FILES = "data/yellow.png"
