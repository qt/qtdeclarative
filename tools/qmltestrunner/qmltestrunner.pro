SOURCES += main.cpp

QT += qml qmltest
CONFIG += no_import_scan

QMAKE_TARGET_PRODUCT = qmltestrunner
QMAKE_TARGET_DESCRIPTION = QML test runner

load(qt_tool)
