option(host_build)

QT = core qmldevtools-private

SOURCES += main.cpp

QMAKE_TARGET_PRODUCT = qmllint
QMAKE_TARGET_DESCRIPTION = QML syntax verifier

load(qt_tool)
