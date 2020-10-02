option(host_build)
TARGET     = QtQmlCompiler
QT         = core-private qmldevtools-private
CONFIG    += internal_module

SOURCES = \
    qqmljsresourcefilemapper.cpp \
    qqmljsimportedmembersvisitor.cpp \
    qqmljsimporter.cpp \
    qqmljstypereader.cpp \
    qqmljsscope.cpp \
    qqmljstypedescriptionreader.cpp \
    qqmljsstreamwriter.cpp

HEADERS = \
    qqmljsresourcefilemapper_p.h \
    qqmljsimportedmembersvisitor_p.h \
    qqmljsimporter_p.h \
    qqmljstypereader_p.h \
    qqmljsmetatypes_p.h \
    qqmljsscope_p.h \
    qqmljstypedescriptionreader_p.h \
    qqmljsstreamwriter_p.h

load(qt_module)
