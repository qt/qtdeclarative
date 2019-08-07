# Generated from qquickwindow.pro.

#####################################################################
## tst_qquickwindow Binary:
#####################################################################

add_qt_executable(tst_qquickwindow
    GUI
    SOURCES
        tst_qquickwindow.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::OpenGLPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::OpenGL
        Qt::Qml
        Qt::Quick
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qquickwindow.pro:<TRUE>:
# CONFIG = "benchmark"
# DEPLOYMENT = "testDataFiles"
# testDataFiles.files = "data"
# testDataFiles.path = "."

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qquickwindow.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
