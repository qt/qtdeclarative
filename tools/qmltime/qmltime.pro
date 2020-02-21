TEMPLATE = app
TARGET = qmltime
QT += qml quick
QT += quick-private
macx:CONFIG -= app_bundle

CONFIG += qmltypes
QML_IMPORT_NAME = QmlTime
QML_IMPORT_VERSION = 1.0

QMAKE_TARGET_DESCRIPTION = QML Time

SOURCES += qmltime.cpp
HEADERS += qmltime.h
