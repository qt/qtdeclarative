include(script.pri)

HEADERS += \
    $$PWD/qv8_p.h \
    $$PWD/qv8debug_p.h \
    $$PWD/qv8profiler_p.h \
    $$PWD/qv8engine_p.h \
    $$PWD/qv8qobjectwrapper_p.h \
    $$PWD/qv8listwrapper_p.h \
    $$PWD/qv8valuetypewrapper_p.h \
    $$PWD/qv4jsonwrapper_p.h \
    $$PWD/qv4domerrors_p.h \
    $$PWD/qv4sqlerrors_p.h \
    $$PWD/qqmlbuiltinfunctions_p.h \
    $$PWD/qv8objectresource_p.h

SOURCES += \
    $$PWD/qv8engine.cpp \
    $$PWD/qv8qobjectwrapper.cpp \
    $$PWD/qv8listwrapper.cpp \
    $$PWD/qv8valuetypewrapper.cpp \
    $$PWD/qv4jsonwrapper.cpp \
    $$PWD/qv4domerrors.cpp \
    $$PWD/qv4sqlerrors.cpp \
    $$PWD/qqmlbuiltinfunctions.cpp

