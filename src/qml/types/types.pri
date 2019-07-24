SOURCES += \
    $$PWD/qqmlbind.cpp \
    $$PWD/qqmlconnections.cpp

HEADERS += \
    $$PWD/qqmlbind_p.h \
    $$PWD/qqmlconnections_p.h

qtConfig(qml-itemmodel) {
    SOURCES += \
        $$PWD/qqmlmodelindexvaluetype.cpp
    HEADERS += \
        $$PWD/qqmlmodelindexvaluetype_p.h
}

qtConfig(qml-animation) {
    SOURCES += \
        $$PWD/qqmltimer.cpp

    HEADERS += \
    $$PWD/qqmltimer_p.h
}
