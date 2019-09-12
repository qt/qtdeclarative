TEMPLATE = lib
CONFIG += plugin qmltypes
QT += qml

QML_IMPORT_NAME = TimeExample
QML_IMPORT_MAJOR_VERSION = 1

DESTDIR = imports/$$QML_IMPORT_NAME
TARGET  = qmlqtimeexampleplugin

SOURCES += \
    plugin.cpp \
    timemodel.cpp

HEADERS += \
    timemodel.h

PLUGINFILES = \
    imports/$$QML_IMPORT_NAME/qmldir \
    imports/$$QML_IMPORT_NAME/center.png \
    imports/$$QML_IMPORT_NAME/clock.png \
    imports/$$QML_IMPORT_NAME/Clock.qml \
    imports/$$QML_IMPORT_NAME/hour.png \
    imports/$$QML_IMPORT_NAME/minute.png

pluginfiles.files += $$PLUGINFILES

qml.files = plugins.qml
qml.path += $$[QT_INSTALL_EXAMPLES]/qml/qmlextensionplugins
target.path += $$[QT_INSTALL_EXAMPLES]/qml/qmlextensionplugins/imports/$$QML_IMPORT_NAME
pluginfiles.path += $$[QT_INSTALL_EXAMPLES]/qml/qmlextensionplugins/imports/$$QML_IMPORT_NAME

INSTALLS += target qml pluginfiles

OTHER_FILES += $$PLUGINFILES plugins.qml

CONFIG += install_ok  # Do not cargo-cult this!
