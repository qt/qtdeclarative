TEMPLATE = app

QT += quick qml

CONFIG += qmltypes
QML_IMPORT_NAME = Example
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += maskedmousearea.h

SOURCES += main.cpp \
           maskedmousearea.cpp

RESOURCES += maskedmousearea.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/maskedmousearea
INSTALLS += target
