QT += qml widgets

CONFIG += qmltypes
QML_IMPORT_NAME = People
QML_IMPORT_MAJOR_VERSION = 1

SOURCES += main.cpp \
           lineedit.cpp
HEADERS += lineedit.h
RESOURCES += extended.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qml/referenceexamples/extended
INSTALLS += target
