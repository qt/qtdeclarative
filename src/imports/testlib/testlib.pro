CXX_MODULE = qml
TARGET  = qmltestplugin
TARGETPATH = QtTest
QML_IMPORT_VERSION = $$QT_VERSION

QT = qmltest-private qml core

SOURCES += \
    main.cpp

QML_FILES = \
    TestCase.qml \
    SignalSpy.qml \
    testlogger.js

load(qml_plugin)
