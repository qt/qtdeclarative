# Generated from librarymetrics_performance.pro.

#####################################################################
## tst_librarymetrics_performance Binary:
#####################################################################

add_qt_benchmark(tst_librarymetrics_performance
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
set(qmake_immediate_resource_files
    "data"
)

add_qt_resource(tst_librarymetrics_performance "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:librarymetrics_performance.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
