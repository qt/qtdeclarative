QT += qml

#![registration]
CONFIG += qmltypes
QML_IMPORT_NAME = io.qt.examples.backend
QML_IMPORT_MAJOR_VERSION = 1
#![registration]

HEADERS += \
    backend.h

SOURCES += \
    backend.cpp \
    main.cpp

RESOURCES += \
    main.qml
