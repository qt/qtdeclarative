QT += qml quick widgets

SOURCES += main.cpp \
    splineeditor.cpp \
    mainwindow.cpp \
    segmentproperties.cpp

RESOURCES = $$PWD/resources.qrc

HEADERS += \
    splineeditor.h \
    mainwindow.h \
    segmentproperties.h

FORMS += \
    properties.ui \
    pane.ui \
    import.ui

QMAKE_TARGET_PRODUCT = qmleasing
QMAKE_TARGET_DESCRIPTION = QML easing curve editor

load(qt_app)
