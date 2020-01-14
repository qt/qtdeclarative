QT += qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = SceneGraphRendering
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += xorblender.h
SOURCES += xorblender.cpp main.cpp

RESOURCES += twotextureproviders.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/twotextureproviders
INSTALLS += target

OTHER_FILES += \
    main.qml
