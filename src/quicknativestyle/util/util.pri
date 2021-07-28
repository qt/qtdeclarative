INCLUDEPATH += $$PWD

macos {
    HEADERS += \
        $$PWD/qquickmacfocusframe.h \

    SOURCES += \
        $$PWD/qquickmacfocusframe.mm \

    RESOURCES += $$PWD/FocusFrame.qml
}
