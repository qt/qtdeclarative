QT += gui-private qml quick
CONFIG += qmltypes
QML_IMPORT_NAME = RhiUnderQML
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += rhisquircle.h
SOURCES += rhisquircle.cpp main.cpp
RESOURCES += rhiunderqml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/rhiunderqml
INSTALLS += target
