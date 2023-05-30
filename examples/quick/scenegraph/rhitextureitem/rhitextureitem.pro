QT += gui-private qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = SceneGraphRendering
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += rhitextureitem.h
SOURCES += rhitextureitem.cpp main.cpp

RESOURCES += rhitextureitem.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/rhitextureitem
INSTALLS += target

OTHER_FILES += \
    main.qml
