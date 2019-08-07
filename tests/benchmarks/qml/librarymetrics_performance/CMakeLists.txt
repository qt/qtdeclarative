# Generated from librarymetrics_performance.pro.

#####################################################################
## tst_librarymetrics_performance Binary:
#####################################################################

add_qt_executable(tst_librarymetrics_performance
    GUI
    SOURCES
        tst_librarymetrics_performance.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
        Qt::Quick
        Qt::Test
)

# Resources:
add_qt_resource(tst_librarymetrics_performance "qmake_immediate"
    PREFIX
        "/"
    FILES
        data
)


#### Keys ignored in scope 1:.:.:librarymetrics_performance.pro:<TRUE>:
# CONFIG = "benchmark" "release"
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:librarymetrics_performance.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
