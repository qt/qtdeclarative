TARGET = qmldbg_inspector
QT    += qml-private quick-private core-private gui-private

INCLUDEPATH *= $$PWD $$PWD/../shared

SOURCES += \
    $$PWD/highlight.cpp \
    $$PWD/qquickviewinspector.cpp \
    $$PWD/abstracttool.cpp \
    $$PWD/abstractviewinspector.cpp \
    $$PWD/inspecttool.cpp \
    $$PWD/qqmlinspectorservice.cpp

HEADERS += \
    $$PWD/highlight.h \
    $$PWD/qquickviewinspector.h \
    $$PWD/qqmlinspectorservicefactory.h \
    $$PWD/abstracttool.h \
    $$PWD/abstractviewinspector.h \
    $$PWD/inspecttool.h

OTHER_FILES += \
    qqmlinspectorservice.json

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QQmlInspectorServiceFactory
load(qt_plugin)
