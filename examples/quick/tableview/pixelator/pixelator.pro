TEMPLATE = app

QT += quick qml
HEADERS += imagemodel.h
SOURCES += main.cpp \
        imagemodel.cpp

CONFIG += qmltypes
QML_IMPORT_NAME = ImageModel
QML_IMPORT_MAJOR_VERSION = 1

RESOURCES += qt.png main.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quick/tableview/pixelator
INSTALLS += target
