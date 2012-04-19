include(script.pri)

HEADERS += \
    $$PWD/qv8_p.h \
    $$PWD/qv8debug_p.h \
    $$PWD/qv8profiler_p.h \
    $$PWD/qv8stringwrapper_p.h \
    $$PWD/qv8engine_p.h \
    $$PWD/qv8sequencewrapper_p.h \
    $$PWD/qv8sequencewrapper_p_p.h \
    $$PWD/qv8contextwrapper_p.h \
    $$PWD/qv8qobjectwrapper_p.h \
    $$PWD/qv8typewrapper_p.h \
    $$PWD/qv8listwrapper_p.h \
    $$PWD/qv8variantwrapper_p.h \
    $$PWD/qv8variantresource_p.h \
    $$PWD/qv8valuetypewrapper_p.h \
    $$PWD/qv8jsonwrapper_p.h \
    $$PWD/qv8include_p.h \
    $$PWD/qv8worker_p.h \
    $$PWD/qv8bindings_p.h \
    $$PWD/qv8engine_impl_p.h \
    $$PWD/qv8domerrors_p.h \
    $$PWD/qv8sqlerrors_p.h \
    $$PWD/qqmlbuiltinfunctions_p.h \
    $$PWD/qv8objectresource_p.h

SOURCES += \
    $$PWD/qv8stringwrapper.cpp \
    $$PWD/qv8engine.cpp \
    $$PWD/qv8sequencewrapper.cpp \
    $$PWD/qv8contextwrapper.cpp \
    $$PWD/qv8qobjectwrapper.cpp \
    $$PWD/qv8typewrapper.cpp \
    $$PWD/qv8listwrapper.cpp \
    $$PWD/qv8variantwrapper.cpp \
    $$PWD/qv8valuetypewrapper.cpp \
    $$PWD/qv8jsonwrapper.cpp \
    $$PWD/qv8include.cpp \
    $$PWD/qv8worker.cpp \
    $$PWD/qv8bindings.cpp \
    $$PWD/qv8domerrors.cpp \
    $$PWD/qv8sqlerrors.cpp \
    $$PWD/qqmlbuiltinfunctions.cpp
