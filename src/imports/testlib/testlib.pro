TARGET  = qmltestplugin
TARGETPATH = QtTest
include(../qimportbase.pri)

CONFIG += qt plugin

symbian {
    CONFIG += epocallowdlldata
    contains(QT_EDITION, OpenSource) {
        TARGET.CAPABILITY = LocalServices NetworkServices ReadUserData UserEnvironment WriteUserData
    } else {
        TARGET.CAPABILITY = All -Tcb
    }
}

QT += declarative script qmltest qmltest-private

SOURCES += main.cpp
HEADERS +=

qdeclarativesources.files += \
    qmldir \
    TestCase.qml \
    SignalSpy.qml \
    testlogger.js

qdeclarativesources.path += $$[QT_INSTALL_IMPORTS]/QtTest
target.path += $$[QT_INSTALL_IMPORTS]/QtTest
INSTALLS += qdeclarativesources target
