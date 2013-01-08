CXX_MODULE = qml
TARGET  = widgetsplugin
TARGETPATH = QtQuick/PrivateWidgets
IMPORT_VERSION = 1.0

SOURCES += \
    widgetsplugin.cpp \
    qquickqfiledialog.cpp \
    ../dialogs/qquickabstractfiledialog.cpp

HEADERS += \
    qquickqfiledialog_p.h \
    ../dialogs/qquickabstractfiledialog_p.h

QT += quick-private gui-private core-private qml-private v8-private widgets

load(qml_plugin)
