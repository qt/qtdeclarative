QT += qml quick

#![0]
CONFIG += qmltypes
QML_IMPORT_NAME = Charts
QML_IMPORT_MAJOR_VERSION = 1
#![0]

HEADERS += piechart.h
SOURCES += piechart.cpp \
           main.cpp

RESOURCES += chapter1-basics.qrc

DESTPATH = $$[QT_INSTALL_EXAMPLES]/qml/tutorials/extending-qml/chapter1-basics
target.path = $$DESTPATH
INSTALLS += target
