CXX_MODULE = qml
TARGET  = dialogplugin
TARGETPATH = QtQuick/Dialogs
IMPORT_VERSION = 1.0

SOURCES += \
    qquickabstractfiledialog.cpp \
    qquickplatformfiledialog.cpp \
    qquickfiledialog.cpp \
    qquickabstractdialog.cpp \
    plugin.cpp

HEADERS += \
    qquickabstractfiledialog_p.h \
    qquickplatformfiledialog_p.h \
    qquickfiledialog_p.h \
    qquickabstractdialog_p.h

QML_FILES += \
    DefaultFileDialog.qml \
    WidgetFileDialog.qml \
    qml/Button.qml \
    qml/TextField.qml \
    qml/qmldir \
    images/folder.png \
    images/titlebar.png \
    images/titlebar.sci \
    images/up.png

QT += quick-private gui-private core-private

load(qml_plugin)
