TEMPLATE = app
CONFIG += qt
QT += qml

SOURCES += window.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/window/window
qml.files = Window.qml nogui.qml standalone.qml twowindows.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/window/window

INSTALLS = target qml
