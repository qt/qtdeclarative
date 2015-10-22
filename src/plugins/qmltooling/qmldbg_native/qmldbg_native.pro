TARGET = qmldbg_native
QT += qml-private core-private

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QQmlNativeDebugConnectorFactory
load(qt_plugin)

SOURCES += \
    $$PWD/qqmlnativedebugconnector.cpp

OTHER_FILES += \
    $$PWD/qqmlnativedebugconnector.json
