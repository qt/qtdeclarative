SOURCES += \
    $$PWD/qqmlbind.cpp \
    $$PWD/qqmlconnections.cpp \
    $$PWD/qqmlmodelindexvaluetype.cpp

HEADERS += \
    $$PWD/qqmlbind_p.h \
    $$PWD/qqmlconnections_p.h \
    $$PWD/qqmlmodelindexvaluetype_p.h

qtConfig(qml-animation) {
    SOURCES += \
        $$PWD/qqmltimer.cpp

    HEADERS += \
    $$PWD/qqmltimer_p.h
}
