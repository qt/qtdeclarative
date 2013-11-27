CXX_MODULE = qml
TARGET  = dialogplugin
TARGETPATH = QtQuick/Dialogs
IMPORT_VERSION = 1.1

QMAKE_DOCS = $$PWD/doc/qtquickdialogs.qdocconf

SOURCES += \
    qquickabstractmessagedialog.cpp \
    qquickplatformmessagedialog.cpp \
    qquickmessagedialog.cpp \
    qquickabstractfiledialog.cpp \
    qquickplatformfiledialog.cpp \
    qquickfiledialog.cpp \
    qquickabstractcolordialog.cpp \
    qquickplatformcolordialog.cpp \
    qquickcolordialog.cpp \
    qquickabstractfontdialog.cpp \
    qquickplatformfontdialog.cpp \
    qquickfontdialog.cpp \
    qquickabstractdialog.cpp \
    plugin.cpp

HEADERS += \
    qquickabstractmessagedialog_p.h \
    qquickplatformmessagedialog_p.h \
    qquickmessagedialog_p.h \
    qquickdialogassets_p.h \
    qquickabstractfiledialog_p.h \
    qquickplatformfiledialog_p.h \
    qquickfiledialog_p.h \
    qquickabstractcolordialog_p.h \
    qquickplatformcolordialog_p.h \
    qquickcolordialog_p.h \
    qquickabstractfontdialog_p.h \
    qquickplatformfontdialog_p.h \
    qquickfontdialog_p.h \
    qquickabstractdialog_p.h

DIALOGS_QML_FILES += \
    DefaultMessageDialog.qml \
    WidgetMessageDialog.qml \
    DefaultFileDialog.qml \
    WidgetFileDialog.qml \
    DefaultColorDialog.qml \
    WidgetColorDialog.qml \
    DefaultFontDialog.qml \
    WidgetFontDialog.qml \
    qml/Button.qml \
    qml/CheckBox.qml \
    qml/ColorSlider.qml \
    qml/EdgeFade.qml \
    qml/DefaultWindowDecoration.qml \
    qml/TextField.qml \
    qml/qmldir \
    images/critical.png \
    images/information.png \
    images/question.png \
    images/warning.png \
    images/checkers.png \
    images/checkmark.png \
    images/copy.png \
    images/crosshairs.png \
    images/slider_handle.png \
    images/sunken_frame.png \
    images/window_border.png \
    images/folder.png \
    images/up.png

QT += quick-private gui gui-private core core-private qml

# Create the resource file
GENERATED_RESOURCE_FILE = $$OUT_PWD/dialogs.qrc

RESOURCE_CONTENT = \
    "<RCC>" \
    "<qresource prefix=\"/QtQuick/Dialogs\">"

for(resourcefile, DIALOGS_QML_FILES) {
    resourcefileabsolutepath = $$absolute_path($$resourcefile)
    relativepath_in = $$relative_path($$resourcefileabsolutepath, $$_PRO_FILE_PWD_)
    relativepath_out = $$relative_path($$resourcefileabsolutepath, $$OUT_PWD)
    RESOURCE_CONTENT += "<file alias=\"$$relativepath_in\">$$relativepath_out</file>"
}

RESOURCE_CONTENT += \
    "</qresource>" \
    "</RCC>"

write_file($$GENERATED_RESOURCE_FILE, RESOURCE_CONTENT)|error("Aborting.")

RESOURCES += $$GENERATED_RESOURCE_FILE

# In case of a debug build, deploy the QML files too
CONFIG(debug, debug|release): QML_FILES += $$DIALOGS_QML_FILES

load(qml_plugin)
