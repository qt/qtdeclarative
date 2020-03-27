TARGET = QtQuickControls2Impl
MODULE = quickcontrols2impl

QT += quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

HEADERS += \
    $$PWD/qtquickcontrols2implglobal_p.h \
    $$PWD/qquickanimatednode_p.h \
    $$PWD/qquickattachedobject_p.h \
    $$PWD/qquickchecklabel_p.h \
    $$PWD/qquickclippedtext_p.h \
    $$PWD/qquickcolor_p.h \
    $$PWD/qquickcolorimage_p.h \
    $$PWD/qquickiconimage_p.h \
    $$PWD/qquickiconimage_p_p.h \
    $$PWD/qquickiconlabel_p.h \
    $$PWD/qquickiconlabel_p_p.h \
    $$PWD/qquickitemgroup_p.h \
    $$PWD/qquickmnemoniclabel_p.h \
    $$PWD/qquickpaddedrectangle_p.h \
    $$PWD/qquickplaceholdertext_p.h \
    $$PWD/qtquickcontrols2foreign_p.h

SOURCES += \
    $$PWD/qquickanimatednode.cpp \
    $$PWD/qquickattachedobject.cpp \
    $$PWD/qquickchecklabel.cpp \
    $$PWD/qquickclippedtext.cpp \
    $$PWD/qquickcolor.cpp \
    $$PWD/qquickcolorimage.cpp \
    $$PWD/qquickiconimage.cpp \
    $$PWD/qquickiconlabel.cpp \
    $$PWD/qquickitemgroup.cpp \
    $$PWD/qquickmnemoniclabel.cpp \
    $$PWD/qquickpaddedrectangle.cpp \
    $$PWD/qquickplaceholdertext.cpp

qtConfig(quick-listview):qtConfig(quick-pathview) {
    HEADERS += \
        $$PWD/qquicktumblerview_p.h
    SOURCES += \
        $$PWD/qquicktumblerview.cpp
}

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/QtQuick/Controls/impl
QML_IMPORT_NAME = QtQuick.Controls.impl
QML_IMPORT_VERSION = 2.15
CONFIG += qmltypes install_qmltypes install_metatypes

load(qt_module)
