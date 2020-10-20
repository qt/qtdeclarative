option(host_build)
TARGET     = QtQmlCompiler
QT         = core-private qmldevtools-private
CONFIG    += internal_module

SOURCES = \
    qqmljsresourcefilemapper.cpp \
    qqmljsimportvisitor.cpp \
    qqmljsimporter.cpp \
    qqmljstypereader.cpp \
    qqmljsscope.cpp \
    qqmljstypedescriptionreader.cpp \
    qqmljsstreamwriter.cpp \
    qresourcerelocater.cpp

HEADERS = \
    qdeferredpointer_p.h \
    qqmljsresourcefilemapper_p.h \
    qqmljsimportvisitor_p.h \
    qqmljsimporter_p.h \
    qqmljstypereader_p.h \
    qqmljsmetatypes_p.h \
    qqmljsscope_p.h \
    qqmljstypedescriptionreader_p.h \
    qqmljsstreamwriter_p.h \
    qresourcerelocater_p.h

load(qt_module)
