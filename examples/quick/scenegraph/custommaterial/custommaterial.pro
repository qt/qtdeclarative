QT += qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = ExampleCustomMaterial
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += customitem.h
SOURCES += customitem.cpp main.cpp

RESOURCES += custommaterial.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/custommaterial
INSTALLS += target

OTHER_FILES += \
    main.qml
