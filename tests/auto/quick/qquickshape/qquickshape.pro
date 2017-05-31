CONFIG += testcase
TARGET = tst_qquickshape
macos:CONFIG -= app_bundle

SOURCES += tst_qquickshape.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

HEADERS += \
    ../../../../src/imports/shapes/qquickshape_p.h \
    ../../../../src/imports/shapes/qquickshape_p_p.h \
    ../../../../src/imports/shapes/qquickshapegenericrenderer_p.h \
    ../../../../src/imports/shapes/qquickshapesoftwarerenderer_p.h

SOURCES += \
    ../../../../src/imports/shapes/qquickshape.cpp \
    ../../../../src/imports/shapes/qquickshapegenericrenderer.cpp \
    ../../../../src/imports/shapes/qquickshapesoftwarerenderer.cpp

qtConfig(opengl) {
    HEADERS += \
        ../../../../src/imports/shapes/qquicknvprfunctions_p.h \
        ../../../../src/imports/shapes/qquicknvprfunctions_p_p.h \
        ../../../../src/imports/shapes/qquickshapenvprrenderer_p.h

    SOURCES += \
        ../../../../src/imports/shapes/qquicknvprfunctions.cpp \
        ../../../../src/imports/shapes/qquickshapenvprrenderer.cpp
}

QT += core-private gui-private  qml-private quick-private testlib
qtHaveModule(widgets): QT += widgets
