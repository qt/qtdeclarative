CXX_MODULE = qml
TARGET  = qmltestplugin
TARGETPATH = QtTest
IMPORT_VERSION = 1.$$QT_MINOR_VERSION

QT += quick qmltest-private qml-private core-private testlib gui-private

SOURCES += \
    main.cpp \
    quicktestevent.cpp \
    quicktestutil.cpp

HEADERS += \
    quicktestevent_p.h \
    quicktestresultforeign_p.h \
    quicktestutil_p.h

QML_FILES = \
    TestCase.qml \
    SignalSpy.qml \
    testlogger.js

load(qml_plugin)

OTHER_FILES += testlib.json
CONFIG += qmltypes install_qmltypes
