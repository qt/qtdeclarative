CXX_MODULE = qml
TARGET  = qmlshapesplugin
TARGETPATH = QtQuick/Shapes
IMPORT_VERSION = 1.0

QT = core gui qml quick quick-private

HEADERS += \
    qquickshape_p.h \
    qquickshape_p_p.h \
    qquickshapegenericrenderer_p.h \
    qquickshapesoftwarerenderer_p.h

SOURCES += \
    plugin.cpp \
    qquickshape.cpp \
    qquickshapegenericrenderer.cpp \
    qquickshapesoftwarerenderer.cpp

qtConfig(opengl) {
    HEADERS += \
        qquicknvprfunctions_p.h \
        qquicknvprfunctions_p_p.h \
        qquickshapenvprrenderer_p.h

    SOURCES += \
        qquicknvprfunctions.cpp \
        qquickshapenvprrenderer.cpp
}

RESOURCES += qtquickshapesplugin.qrc

load(qml_plugin)
