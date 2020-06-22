
QT += quick

CONFIG += qmltypes
QML_IMPORT_NAME = SimpleMaterial
QML_IMPORT_MAJOR_VERSION = 1

SOURCES += \
    simplematerial.cpp \
    simplematerialitem.cpp
RESOURCES += simplematerial.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/simplematerial
INSTALLS += target

HEADERS += \
    simplematerialitem.h
