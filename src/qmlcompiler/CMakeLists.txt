# Generated from qmlcompiler.pro.

#####################################################################
## QmlCompiler Module:
#####################################################################

qt_add_module(QmlCompiler
    STATIC
    INTERNAL_MODULE
    SOURCES
        importedmembersvisitor.cpp importedmembersvisitor_p.h
        qmljsimporter.cpp qmljsimporter_p.h
        qmljstypereader.cpp qmljstypereader_p.h
        qmlstreamwriter.cpp qmlstreamwriter_p.h
        qqmljsmetatypes_p.h
        qqmljsscope.cpp qqmljsscope_p.h
        resourcefilemapper.cpp resourcefilemapper_p.h
        typedescriptionreader.cpp typedescriptionreader_p.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmlcompiler.pro:<TRUE>:
# _OPTION = "host_build"
