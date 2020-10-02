# Generated from qmlcompiler.pro.

#####################################################################
## QmlCompiler Module:
#####################################################################

qt_add_module(QmlCompiler
    STATIC
    INTERNAL_MODULE
    SOURCES
        qqmljsimportedmembersvisitor.cpp qqmljsimportedmembersvisitor_p.h
        qqmljsimporter.cpp qqmljsimporter_p.h
        qqmljsmetatypes_p.h
        qqmljsresourcefilemapper.cpp qqmljsresourcefilemapper_p.h
        qqmljsscope.cpp qqmljsscope_p.h
        qqmljsstreamwriter.cpp qqmljsstreamwriter_p.h
        qqmljstypedescriptionreader.cpp qqmljstypedescriptionreader_p.h
        qqmljstypereader.cpp qqmljstypereader_p.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmlcompiler.pro:<TRUE>:
# _OPTION = "host_build"
