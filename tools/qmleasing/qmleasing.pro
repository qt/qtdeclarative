QT += qml quick widgets
CONFIG -= app_bundle

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
