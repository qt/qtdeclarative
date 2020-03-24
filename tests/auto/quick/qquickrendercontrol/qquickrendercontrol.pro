CONFIG += testcase
TARGET = tst_qquickrendercontrol
SOURCES += tst_qquickrendercontrol.cpp

include (../../shared/util.pri)

macos:CONFIG -= app_bundle

QT += core-private gui-private qml-private quick-private testlib opengl

OTHER_FILES += \
    data/rect.qml
