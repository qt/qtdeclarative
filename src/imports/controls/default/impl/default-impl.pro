TARGET = qtquickcontrols2defaultstyleimplplugin
TARGETPATH = QtQuick/Controls/Default/impl

QML_IMPORT_NAME = QtQuick.Controls.Default.impl
QML_IMPORT_VERSION = $$QT_VERSION

QT += qml quick
QT_PRIVATE += core-private gui qml-private quick-private quicktemplates2-private quickcontrols2impl-private
QT_FOR_CONFIG = quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

HEADERS += \
    $$PWD/qquickdefaultbusyindicator_p.h \
    $$PWD/qquickdefaultdial_p.h \
    $$PWD/qquickdefaultprogressbar_p.h

SOURCES += \
    $$PWD/qquickdefaultbusyindicator.cpp \
    $$PWD/qquickdefaultdial.cpp \
    $$PWD/qquickdefaultprogressbar.cpp \
    $$PWD/qtquickcontrols2defaultstyleimplplugin.cpp

CONFIG += qmltypes install_qmltypes no_cxx_module
load(qml_plugin)

requires(qtConfig(quickcontrols2-default))
