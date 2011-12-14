HEADERS +=  \
    $$PWD/qbitfield_p.h \
    $$PWD/qintrusivelist_p.h \
    $$PWD/qpodvector_p.h \
    $$PWD/qhashedstring_p.h \
    $$PWD/qdeclarativerefcount_p.h \
    $$PWD/qdeclarativepool_p.h \
    $$PWD/qfieldlist_p.h \
    $$PWD/qfastmetabuilder_p.h \
    $$PWD/qhashfield_p.h \
    $$PWD/qdeclarativethread_p.h \
    $$PWD/qfinitestack_p.h \
    $$PWD/qrecursionwatcher_p.h \
    $$PWD/qdeletewatcher_p.h \
    $$PWD/qrecyclepool_p.h \
    $$PWD/qflagpointer_p.h \
    $$PWD/qdeclarativetrace_p.h \

SOURCES += \
    $$PWD/qintrusivelist.cpp \
    $$PWD/qhashedstring.cpp \
    $$PWD/qdeclarativepool.cpp \
    $$PWD/qfastmetabuilder.cpp \
    $$PWD/qdeclarativethread.cpp \
    $$PWD/qdeclarativetrace.cpp \

contains(QT_CONFIG, clock-gettime):include($$QT_SOURCE_TREE/config.tests/unix/clock-gettime/clock-gettime.pri)
