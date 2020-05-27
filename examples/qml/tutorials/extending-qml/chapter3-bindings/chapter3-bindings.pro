QT += qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = Charts
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += piechart.h
SOURCES += piechart.cpp \
           main.cpp

RESOURCES += chapter3-bindings.qrc

DESTPATH = $$[QT_INSTALL_EXAMPLES]/qml/tutorials/extending-qml/chapter3-bindings
target.path = $$DESTPATH
INSTALLS += target
