# Generated from qmllint.pro.

#####################################################################
## qmllint Tool:
#####################################################################

qt_get_tool_target_name(target_name qmllint)
qt_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Syntax Verifier"
    TOOLS_TARGET Qml # special case
    SOURCES
        ../shared/importedmembersvisitor.cpp ../shared/importedmembersvisitor.h
        ../shared/metatypes.h
        ../shared/qmljsimporter.cpp ../shared/qmljsimporter.h
        ../shared/qmljstypereader.cpp ../shared/qmljstypereader.h
        ../shared/scopetree.cpp ../shared/scopetree.h
        ../shared/typedescriptionreader.cpp ../shared/typedescriptionreader.h
        checkidentifiers.cpp checkidentifiers.h
        findwarnings.cpp findwarnings.h
        main.cpp
        qcoloroutput.cpp qcoloroutput.h
    INCLUDE_DIRECTORIES
        ../shared
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlDevToolsPrivate
)

#### Keys ignored in scope 1:.:.:qmllint.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Syntax" "Verifier"
# _OPTION = "host_build"
