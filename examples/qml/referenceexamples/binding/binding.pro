QT += qml

CONFIG += qmltypes
QML_IMPORT_NAME = People
QML_IMPORT_MAJOR_VERSION = 1

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp \
           happybirthdaysong.cpp
HEADERS += person.h \
           birthdayparty.h \
           happybirthdaysong.h
RESOURCES += binding.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qml/referenceexamples/binding
INSTALLS += target
