TARGET = qtquickcontrols2fusionstyleimplplugin
TARGETPATH = QtQuick/Controls/Fusion/impl

QML_IMPORT_NAME = QtQuick.Controls.Fusion.impl
QML_IMPORT_VERSION = 2.$$QT_MINOR_VERSION

QT += qml quick
QT_PRIVATE += core-private gui qml-private quick-private quicktemplates2-private quickcontrols2impl-private
QT_FOR_CONFIG = quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QML_FILES += \
    $$PWD/ButtonPanel.qml \
    $$PWD/CheckIndicator.qml \
    $$PWD/RadioIndicator.qml \
    $$PWD/SliderGroove.qml \
    $$PWD/SliderHandle.qml \
    $$PWD/SwitchIndicator.qml

OTHER_FILES += \
    qmldir \
    $$QML_FILES

HEADERS += \
    $$PWD/qquickfusionbusyindicator_p.h \
    $$PWD/qquickfusiondial_p.h \
    $$PWD/qquickfusionknob_p.h

SOURCES += \
    $$PWD/qquickfusionbusyindicator.cpp \
    $$PWD/qquickfusiondial.cpp \
    $$PWD/qquickfusionknob.cpp \
    $$PWD/qtquickcontrols2fusionstyleimplplugin.cpp

CONFIG += qmltypes install_qmltypes no_cxx_module install_qml_files builtin_resources qtquickcompiler
load(qml_plugin)

requires(qtConfig(quickcontrols2-fusion))
