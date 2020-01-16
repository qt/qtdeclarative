!macos:!ios: error("This example requires macOS or iOS")

QT += qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = MetalUnderQML
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += metalsquircle.h
SOURCES += metalsquircle.mm main.cpp
RESOURCES += metalunderqml.qrc

LIBS += -framework Metal
macos: LIBS += -framework AppKit

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/metalunderqml
INSTALLS += target
