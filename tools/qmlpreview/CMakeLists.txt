# Generated from qmlpreview.pro.

#####################################################################
## qmlpreview Tool:
#####################################################################

add_qt_tool(qmlpreview
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
        qmlpreviewapplication.cpp qmlpreviewapplication.h
        qmlpreviewfilesystemwatcher.cpp qmlpreviewfilesystemwatcher.h
    LIBRARIES
        Qt::QmlDebugPrivate
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::QmlDebug
)

#### Keys ignored in scope 1:.:.:qmlpreview.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Preview"
