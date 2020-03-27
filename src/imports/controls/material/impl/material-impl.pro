TARGET = qtquickcontrols2materialstyleimplplugin
TARGETPATH = QtQuick/Controls.2/Material/impl

QML_IMPORT_NAME = QtQuick.Controls.Material.impl
QML_IMPORT_VERSION = 2.$$QT_MINOR_VERSION

QT += qml quick
QT_PRIVATE += core-private gui qml-private quick-private quicktemplates2-private quickcontrols2impl-private
QT_FOR_CONFIG = quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QML_FILES += \
    $$PWD/BoxShadow.qml \
    $$PWD/CheckIndicator.qml \
    $$PWD/CursorDelegate.qml \
    $$PWD/ElevationEffect.qml \
    $$PWD/RadioIndicator.qml \
    $$PWD/RectangularGlow.qml \
    $$PWD/SliderHandle.qml \
    $$PWD/SwitchIndicator.qml

OTHER_FILES += \
    qmldir \
    $$QML_FILES

HEADERS += \
    qquickmaterialbusyindicator_p.h \
    qquickmaterialprogressbar_p.h \
    qquickmaterialripple_p.h

SOURCES += \
    $$PWD/qquickmaterialbusyindicator.cpp \
    $$PWD/qquickmaterialprogressbar.cpp \
    $$PWD/qquickmaterialripple.cpp \
    $$PWD/qtquickcontrols2materialstyleimplplugin.cpp

CONFIG += qmltypes install_qmltypes no_cxx_module install_qml_files builtin_resources qtquickcompiler
load(qml_plugin)

requires(qtConfig(quickcontrols2-material))
