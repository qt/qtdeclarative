CONFIG += testcase
TARGET = tst_qquickpathitem
macx:CONFIG -= app_bundle

SOURCES += tst_qquickpathitem.cpp

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

HEADERS += \
    ../../../../src/imports/pathitem/qquickpathitem_p.h \
    ../../../../src/imports/pathitem/qquickpathitem_p_p.h \
    ../../../../src/imports/pathitem/qquickpathitemgenericrenderer_p.h \
    ../../../../src/imports/pathitem/qquickpathitemsoftwarerenderer_p.h

SOURCES += \
    ../../../../src/imports/pathitem/qquickpathitem.cpp \
    ../../../../src/imports/pathitem/qquickpathitemgenericrenderer.cpp \
    ../../../../src/imports/pathitem/qquickpathitemsoftwarerenderer.cpp

qtConfig(opengl) {
    HEADERS += \
        ../../../../src/imports/pathitem/qquicknvprfunctions_p.h \
        ../../../../src/imports/pathitem/qquicknvprfunctions_p_p.h \
        ../../../../src/imports/pathitem/qquickpathitemnvprrenderer_p.h

    SOURCES += \
        ../../../../src/imports/pathitem/qquicknvprfunctions.cpp \
        ../../../../src/imports/pathitem/qquickpathitemnvprrenderer.cpp
}

QT += core-private gui-private  qml-private quick-private testlib
qtHaveModule(widgets): QT += widgets
