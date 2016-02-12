TARGET = qmldbg_debugger
QT = qml-private core-private

SOURCES += \
    $$PWD/qdebugmessageservice.cpp \
    $$PWD/qqmldebuggerservicefactory.cpp \
    $$PWD/qqmlenginedebugservice.cpp \
    $$PWD/qqmlnativedebugservice.cpp \
    $$PWD/qqmlwatcher.cpp \
    $$PWD/qv4debugservice.cpp \
    $$PWD/qv4debuggeragent.cpp \
    $$PWD/qv4datacollector.cpp

HEADERS += \
    $$PWD/../shared/qqmlconfigurabledebugservice.h \
    $$PWD/qdebugmessageservice.h \
    $$PWD/qqmldebuggerservicefactory.h \
    $$PWD/qqmlenginedebugservice.h \
    $$PWD/qqmlnativedebugservice.h \
    $$PWD/qqmlwatcher.h \
    $$PWD/qv4debugservice.h \
    $$PWD/qv4debuggeragent.h \
    $$PWD/qv4datacollector.h

INCLUDEPATH += $$PWD \
    $$PWD/../shared

OTHER_FILES += \
    $$PWD/qqmldebuggerservice.json

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QQmlDebuggerServiceFactory
load(qt_plugin)
