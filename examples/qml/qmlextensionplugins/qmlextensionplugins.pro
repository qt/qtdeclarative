TEMPLATE = lib
CONFIG += plugin qmltypes
QT += qml

QML_IMPORT_NAME = TimeExample
QML_IMPORT_MAJOR_VERSION = 1

DESTDIR = imports/$$QML_IMPORT_NAME
TARGET  = qmlqtimeexampleplugin
QMLTYPES_FILENAME = $$DESTDIR/plugins.qmltypes

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

target.path = $$[QT_INSTALL_EXAMPLES]/qml/qmlextensionplugins/imports/$$QML_IMPORT_NAME

pluginfiles_copy.files = $$PLUGINFILES
pluginfiles_copy.path = $$DESTDIR

pluginfiles_install.files = $$PLUGINFILES $$OUT_PWD/$$DESTDIR/plugins.qmltypes
pluginfiles_install.path = $$[QT_INSTALL_EXAMPLES]/qml/qmlextensionplugins/imports/$$QML_IMPORT_NAME

qml_copy.files = plugins.qml plugins.qmlproject
qml_copy.path = $$OUT_PWD

qml_install.files = plugins.qml plugins.qmlproject
qml_install.path = $$[QT_INSTALL_EXAMPLES]/qml/qmlextensionplugins

INSTALLS += target qml_install pluginfiles_install
COPIES += qml_copy pluginfiles_copy

OTHER_FILES += $$PLUGINFILES plugins.qml

CONFIG += install_ok  # Do not cargo-cult this!
