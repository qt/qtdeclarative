QT += qml gui

#![0]
CONFIG += qmltypes
QML_IMPORT_NAME = Qt.example
QML_IMPORT_MAJOR_VERSION = 1
#![0]

RESOURCES += \
    exampleFive.qml \
    exampleFour.js \
    exampleFour.qml \
    exampleOne.qml \
    exampleThree.js \
    exampleThree.qml \
    exampleTwo.qml

HEADERS += \
    avatarExample.h

SOURCES += \
    avatarExample.cpp
