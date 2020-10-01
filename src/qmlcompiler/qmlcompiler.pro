option(host_build)
TARGET     = QtQmlCompiler
QT         = core-private qmldevtools-private
CONFIG    += internal_module

SOURCES = \
    resourcefilemapper.cpp \
    importedmembersvisitor.cpp \
    qmljsimporter.cpp \
    qmljstypereader.cpp \
    qqmljsscope.cpp \
    typedescriptionreader.cpp \
    qmlstreamwriter.cpp

HEADERS = \
    resourcefilemapper_p.h \
    importedmembersvisitor_p.h \
    qmljsimporter_p.h \
    qmljstypereader_p.h \
    metatypes_p.h \
    qqmljsscope_p.h \
    typedescriptionreader_p.h \
    qmlstreamwriter_p.h

load(qt_module)
