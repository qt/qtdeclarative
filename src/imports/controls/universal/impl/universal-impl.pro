TARGET = qtquickcontrols2universalstyleimplplugin
TARGETPATH = QtQuick/Controls/Universal/impl

QML_IMPORT_NAME = QtQuick.Controls.Universal.impl
QML_IMPORT_VERSION = $$QT_VERSION

QT += qml quick
QT_PRIVATE += core-private gui qml-private quick-private quicktemplates2-private quickcontrols2impl-private
QT_FOR_CONFIG = quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QML_FILES += \
    $$PWD/CheckIndicator.qml \
    $$PWD/RadioIndicator.qml \
    $$PWD/SwitchIndicator.qml

OTHER_FILES += \
    qmldir \
    $$QML_FILES

HEADERS += \
    $$PWD/qquickuniversalbusyindicator_p.h \
    $$PWD/qquickuniversalfocusrectangle_p.h \
    $$PWD/qquickuniversalprogressbar_p.h

SOURCES += \
    $$PWD/qquickuniversalbusyindicator.cpp \
    $$PWD/qquickuniversalfocusrectangle.cpp \
    $$PWD/qquickuniversalprogressbar.cpp \
    $$PWD/qtquickcontrols2universalstyleimplplugin.cpp

CONFIG += qmltypes install_qmltypes no_cxx_module install_qml_files builtin_resources qtquickcompiler
load(qml_plugin)

requires(qtConfig(quickcontrols2-universal))
