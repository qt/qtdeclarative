INCLUDEPATH += $$PWD/../../../3rdparty/javascriptcore
INCLUDEPATH += $$PWD

include(script.pri)

HEADERS += \
    $$PWD/qv8_p.h \
    $$PWD/qv8stringwrapper_p.h \
    $$PWD/qv8engine_p.h \
    $$PWD/qhashedstring_p.h \
    $$PWD/qv8contextwrapper_p.h \
    $$PWD/qv8qobjectwrapper_p.h \
    $$PWD/qv8typewrapper_p.h \
    $$PWD/qv8listwrapper_p.h \
    $$PWD/qv8variantwrapper_p.h \
    $$PWD/qv8valuetypewrapper_p.h \
    $$PWD/qv8include_p.h \
    $$PWD/qv8worker_p.h \
    $$PWD/qv8bindings_p.h \
    $$PWD/../../../3rdparty/javascriptcore/DateMath.h \
    $$PWD/qv8engine_impl_p.h

SOURCES += \
    $$PWD/qv8stringwrapper.cpp \
    $$PWD/qv8engine.cpp \
    $$PWD/qhashedstring.cpp \
    $$PWD/qv8contextwrapper.cpp \
    $$PWD/qv8qobjectwrapper.cpp \
    $$PWD/qv8typewrapper.cpp \
    $$PWD/qv8listwrapper.cpp \
    $$PWD/qv8variantwrapper.cpp \
    $$PWD/qv8valuetypewrapper.cpp \
    $$PWD/qv8include.cpp \
    $$PWD/qv8worker.cpp \
    $$PWD/qv8bindings.cpp \
    $$PWD/../../../3rdparty/javascriptcore/DateMath.cpp \
