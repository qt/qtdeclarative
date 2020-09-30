INCLUDEPATH += $$PWD

# The relevant tools need different bits and pieces.
# Furthermore, some of the classes require devtools, some not.

RESOURCEFILEMAPPER_SOURCES = \
    $$PWD/resourcefilemapper.cpp

RESOURCEFILEMAPPER_HEADERS = \
    $$PWD/resourcefilemapper.h

METATYPEREADER_SOURCES = \
    $$PWD/importedmembersvisitor.cpp \
    $$PWD/qmljsimporter.cpp \
    $$PWD/qmljstypereader.cpp \
    $$PWD/scopetree.cpp \
    $$PWD/typedescriptionreader.cpp

METATYPEREADER_HEADERS = \
    $$PWD/importedmembersvisitor.h \
    $$PWD/qmljsimporter.h \
    $$PWD/qmljstypereader.h \
    $$PWD/metatypes.h \
    $$PWD/scopetree.h \
    $$PWD/typedescriptionreader.h

QMLSTREAMWRITER_SOURCES = \
    $$PWD/qmlstreamwriter.cpp

QMLSTREAMWRITER_HEADERS = \
    $$PWD/qmlstreamwriter.h
