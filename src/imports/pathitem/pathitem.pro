CXX_MODULE = qml
TARGET  = qmlpathitemplugin
TARGETPATH = Qt/labs/pathitem
IMPORT_VERSION = 1.0

QT = core gui qml quick quick-private

HEADERS += \
    qquickpathitem_p.h \
    qquickpathitem_p_p.h \
    qquickpathitemgenericrenderer_p.h \
    qquickpathitemsoftwarerenderer_p.h

SOURCES += \
    plugin.cpp \
    qquickpathitem.cpp \
    qquickpathitemgenericrenderer.cpp \
    qquickpathitemsoftwarerenderer.cpp

qtConfig(opengl) {
    HEADERS += \
        qquicknvprfunctions_p.h \
        qquicknvprfunctions_p_p.h \
        qquickpathitemnvprrenderer_p.h

    SOURCES += \
        qquicknvprfunctions.cpp \
        qquickpathitemnvprrenderer.cpp
}

load(qml_plugin)
