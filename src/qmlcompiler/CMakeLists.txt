# Generated from qmlcompiler.pro.

#####################################################################
## QmlCompiler Module:
#####################################################################

qt_add_module(QmlCompiler
    STATIC
    INTERNAL_MODULE
    SOURCES
        qmljsimporter.cpp qmljsimporter_p.h
        qmljstypereader.cpp qmljstypereader_p.h
        qqmljsimportedmembersvisitor.cpp qqmljsimportedmembersvisitor_p.h
        qqmljsmetatypes_p.h
        qqmljsresourcefilemapper.cpp qqmljsresourcefilemapper_p.h
        qqmljsscope.cpp qqmljsscope_p.h
        qqmljsstreamwriter.cpp qqmljsstreamwriter_p.h
        typedescriptionreader.cpp typedescriptionreader_p.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmlcompiler.pro:<TRUE>:
# _OPTION = "host_build"
