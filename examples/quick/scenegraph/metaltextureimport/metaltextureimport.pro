!macos: error("This example requires macOS")

QT += qml quick

HEADERS += metaltextureimport.h
SOURCES += metaltextureimport.mm main.cpp
RESOURCES += metaltextureimport.qrc

LIBS += -framework Metal -framework AppKit

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/metaltextureimport
INSTALLS += target
