!macos:!ios: error("This example requires macOS or iOS")

QT += qml quick
CONFIG += qmltypes
QML_IMPORT_NAME = MetalTextureImport
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += metaltextureimport.h
SOURCES += metaltextureimport.mm main.cpp
RESOURCES += metaltextureimport.qrc

LIBS += -framework Metal
macos: LIBS += -framework AppKit

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/metaltextureimport
INSTALLS += target
