option(host_build)
QT       = core qmldevtools-private
SOURCES += main.cpp

QMAKE_TARGET_PRODUCT = qmlmin
QMAKE_TARGET_DESCRIPTION = QML/JS minifier

load(qt_tool)
