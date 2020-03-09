# Generated from qmllint.pro.

#####################################################################
## qmllint Tool:
#####################################################################

qt_add_tool(qmllint
    SOURCES
        componentversion.cpp componentversion.h
        findunqualified.cpp findunqualified.h
        importedmembersvisitor.cpp importedmembersvisitor.h
        main.cpp
        metatypes.h
        qcoloroutput.cpp qcoloroutput.h
        scopetree.cpp scopetree.h
        typedescriptionreader.cpp typedescriptionreader.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmllint.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Syntax" "Verifier"
# _OPTION = "host_build"
