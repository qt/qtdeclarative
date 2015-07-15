TARGET = qmldbg_inspector
QT    += qml-private quick-private core-private gui-private

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QQmlInspectorServiceFactory
load(qt_plugin)

INCLUDEPATH *= $$PWD $$PWD/../shared

SOURCES += \
    $$PWD/qtquick2plugin.cpp \
    $$PWD/highlight.cpp \
    $$PWD/qquickviewinspector.cpp \
    $$PWD/abstracttool.cpp \
    $$PWD/abstractviewinspector.cpp \
    $$PWD/inspecttool.cpp \
    $$PWD/qqmlinspectorservice.cpp

HEADERS += \
    $$PWD/qtquick2plugin.h \
    $$PWD/highlight.h \
    $$PWD/qquickviewinspector.h \
    $$PWD/qqmlinspectorservice.h \
    $$PWD/abstracttool.h \
    $$PWD/abstractviewinspector.h \
    $$PWD/qqmlinspectorinterface.h \
    $$PWD/inspecttool.h

OTHER_FILES += \
    qqmlinspectorservice.json
