TEMPLATE = lib
CONFIG += qt plugin
QT += qml

DESTDIR = imports/TimeExample
TARGET  = qmlqtimeexampleplugin

SOURCES += plugin.cpp

qdeclarativesources.files += \
    imports/TimeExample/qmldir \
    imports/TimeExample/center.png \
    imports/TimeExample/clock.png \
    imports/TimeExample/Clock.qml \
    imports/TimeExample/hour.png \
    imports/TimeExample/minute.png

qdeclarativesources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/plugins/imports/TimeExample

sources.files += plugins.pro plugin.cpp plugins.qml
sources.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/plugins
target.path += $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/plugins/imports/TimeExample

INSTALLS += qdeclarativesources sources target

