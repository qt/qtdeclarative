QT += qml

win32: CONFIG += console
mac:CONFIG -= app_bundle

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/shell
sources.files = $$SOURCES shell.pro
sources.path = $$target.path
INSTALLS += target sources
