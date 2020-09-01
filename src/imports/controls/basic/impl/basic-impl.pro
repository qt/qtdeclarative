TARGET = qtquickcontrols2basicstyleimplplugin
TARGETPATH = QtQuick/Controls/Basic/impl

QML_IMPORT_NAME = QtQuick.Controls.Basic.impl
QML_IMPORT_VERSION = $$QT_VERSION

QT += qml quick
QT_PRIVATE += core-private gui qml-private quick-private quicktemplates2-private quickcontrols2impl-private
QT_FOR_CONFIG = quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

HEADERS += \
    $$PWD/qquickbasicbusyindicator_p.h \
    $$PWD/qquickbasicdial_p.h \
    $$PWD/qquickbasicprogressbar_p.h

SOURCES += \
    $$PWD/qquickbasicbusyindicator.cpp \
    $$PWD/qquickbasicdial.cpp \
    $$PWD/qquickbasicprogressbar.cpp \
    $$PWD/qtquickcontrols2basicstyleimplplugin.cpp

CONFIG += qmltypes install_qmltypes no_cxx_module
load(qml_plugin)

requires(qtConfig(quickcontrols2-basic))
