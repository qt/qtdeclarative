!macos:!ios: error("This example requires macOS or iOS")

QT += qml quick

HEADERS += metaltextureimport.h
SOURCES += metaltextureimport.mm main.cpp
RESOURCES += metaltextureimport.qrc

LIBS += -framework Metal
macos: LIBS += -framework AppKit

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/metaltextureimport
INSTALLS += target
