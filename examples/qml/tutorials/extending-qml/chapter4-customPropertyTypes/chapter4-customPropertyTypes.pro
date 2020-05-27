QT += qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = Charts
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += piechart.h \
           pieslice.h
SOURCES += piechart.cpp \
           pieslice.cpp \
           main.cpp

RESOURCES += chapter4-customPropertyTypes.qrc

DESTPATH = $$[QT_INSTALL_EXAMPLES]/qml/tutorials/extending-qml/chapter4-customPropertyTypes
target.path = $$DESTPATH
INSTALLS += target
