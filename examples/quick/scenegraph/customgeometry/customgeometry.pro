TARGET = customgeometry
QT += quick

CONFIG += qmltypes
QML_IMPORT_NAME = CustomGeometry
QML_IMPORT_MAJOR_VERSION = 1

SOURCES += \
    main.cpp \
    beziercurve.cpp

HEADERS += \
    beziercurve.h

RESOURCES += customgeometry.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/customgeometry
INSTALLS += target
