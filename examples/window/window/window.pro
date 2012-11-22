TEMPLATE = app
CONFIG += qt
QT += qml

SOURCES += window.cpp
OTHER_FILES = Window.qml nogui.qml standalone.qml twowindows.qml

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/window/window
sources.files = $$SOURCES $$OTHER_FILES window.pro
sources.path = $$target.path
INSTALLS = sources target
