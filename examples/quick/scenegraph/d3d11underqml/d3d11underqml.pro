!win32: error("This example requires Windows")

QT += qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = D3D11UnderQML
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += d3d11squircle.h
SOURCES += d3d11squircle.cpp main.cpp
RESOURCES += d3d11underqml.qrc

LIBS += -ld3d11 -ld3dcompiler

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/d3d11underqml
INSTALLS += target
