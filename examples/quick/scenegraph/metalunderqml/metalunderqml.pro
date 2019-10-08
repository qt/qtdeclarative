!macos: error("This example requires macOS")

QT += qml quick

HEADERS += metalsquircle.h
SOURCES += metalsquircle.mm main.cpp
RESOURCES += metalunderqml.qrc

LIBS += -framework Metal -framework AppKit

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/metalunderqml
INSTALLS += target
