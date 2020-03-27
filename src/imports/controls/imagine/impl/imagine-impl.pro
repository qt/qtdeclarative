TARGET = qtquickcontrols2imaginestyleimplplugin
TARGETPATH = QtQuick/Controls.2/Imagine/impl

QML_IMPORT_NAME = QtQuick.Controls.Imagine.impl
QML_IMPORT_VERSION = 2.$$QT_MINOR_VERSION

QT += qml quick
QT_PRIVATE += core-private gui qml-private quick-private quicktemplates2-private quickcontrols2impl-private
QT_FOR_CONFIG = quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QML_FILES += \
    $$PWD/OpacityMask.qml

OTHER_FILES += \
    qmldir \
    $$QML_FILES

HEADERS += \
    $$PWD/qquickimageselector_p.h \
    $$PWD/qquickninepatchimage_p.h

SOURCES += \
    $$PWD/qquickimageselector.cpp \
    $$PWD/qquickninepatchimage.cpp \
    $$PWD/qtquickcontrols2imaginestyleimplplugin.cpp

qtquickcontrols2imaginestyleimpl.prefix = qt-project.org/imports/QtQuick/Controls/Imagine/impl
qtquickcontrols2imaginestyleimpl.files += \
    $$files($$PWD/shaders/OpacityMask.frag) \
    $$files($$PWD/shaders/+glslcore/OpacityMask.frag) \
    $$files($$PWD/shaders/+qsb/OpacityMask.frag)
RESOURCES += qtquickcontrols2imaginestyleimpl

CONFIG += qmltypes install_qmltypes no_cxx_module builtin_resources qtquickcompiler
load(qml_plugin)

requires(qtConfig(quickcontrols2-imagine))
