QT += qml quick

HEADERS += documenthighlighter.h
SOURCES += main.cpp \
           documenthighlighter.cpp

RESOURCES += syntaxhighlighter.qrc
CONFIG += qmltypes
QML_IMPORT_NAME = Highlighter
QML_IMPORT_MAJOR_VERSION = 1
