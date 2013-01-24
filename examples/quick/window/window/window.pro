TEMPLATE = app
QT += qml

SOURCES += window.cpp
RESOURCES += window.qrc

EXAMPLE_FILES = \
    nogui.qml \
    standalone.qml \
    twowindows.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quick/window/window
INSTALLS = target
