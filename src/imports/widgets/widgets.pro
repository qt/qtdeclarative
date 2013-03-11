CXX_MODULE = qml
TARGET  = widgetsplugin
TARGETPATH = QtQuick/PrivateWidgets
IMPORT_VERSION = 1.0

SOURCES += \
    qquickqfiledialog.cpp \
    ../dialogs/qquickabstractfiledialog.cpp \
    qquickqcolordialog.cpp \
    ../dialogs/qquickabstractcolordialog.cpp \
    ../dialogs/qquickabstractdialog.cpp \
    widgetsplugin.cpp

HEADERS += \
    qquickqfiledialog_p.h \
    ../dialogs/qquickabstractfiledialog_p.h \
    qquickqcolordialog_p.h \
    ../dialogs/qquickabstractcolordialog_p.h \
    ../dialogs/qquickabstractdialog_p.h

QT += quick-private gui-private core-private qml-private v8-private widgets

load(qml_plugin)
