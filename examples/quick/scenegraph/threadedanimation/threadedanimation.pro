QT += qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = Spinner
QML_IMPORT_MAJOR_VERSION = 1

HEADERS += spinner.h
SOURCES += spinner.cpp main.cpp
RESOURCES += threadedanimation.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/threadedanimation
INSTALLS += target
