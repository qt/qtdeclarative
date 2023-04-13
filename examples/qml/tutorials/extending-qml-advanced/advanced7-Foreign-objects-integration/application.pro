TEMPLATE = app

CONFIG += console

QT += core qml

DEPENDPATH += library
INCLUDEPATH += library
LIBS += -Llibrary/ -llibrary

SOURCES += \
    birthdayparty.cpp \
    happybirthdaysong.cpp \
    main.cpp \
    person.cpp
HEADERS += \
    birthdayparty.h \
    foreigndisplay.h \
    happybirthdaysong.h \
    person.h

CONFIG += qmltypes
QML_IMPORT_NAME = People
QML_IMPORT_MAJOR_VERSION = 1

RESOURCES += foreign.qrc
