option(host_build)
TARGET     = QtQmlCompiler
QT         = core-private qmldevtools-private
CONFIG    += internal_module

SOURCES = \
    qqmljsresourcefilemapper.cpp \
    qqmljsimportedmembersvisitor.cpp \
    qmljsimporter.cpp \
    qmljstypereader.cpp \
    qqmljsscope.cpp \
    typedescriptionreader.cpp \
    qqmljsstreamwriter.cpp

HEADERS = \
    qqmljsresourcefilemapper_p.h \
    qqmljsimportedmembersvisitor_p.h \
    qmljsimporter_p.h \
    qmljstypereader_p.h \
    qqmljsmetatypes_p.h \
    qqmljsscope_p.h \
    typedescriptionreader_p.h \
    qqmljsstreamwriter_p.h

load(qt_module)
