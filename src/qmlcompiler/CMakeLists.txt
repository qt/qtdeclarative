qt_internal_add_module(QmlCompilerPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        qcoloroutput_p.h qcoloroutput.cpp
        qdeferredpointer_p.h
        qqmljslogger_p.h qqmljslogger.cpp
        qqmljscompiler.cpp qqmljscompiler_p.h
        qqmljsimporter.cpp qqmljsimporter_p.h
        qqmljsimportvisitor.cpp qqmljsimportvisitor_p.h
        qqmljsloadergenerator.cpp qqmljsloadergenerator_p.h
        qqmljsmetatypes_p.h
        qqmljsresourcefilemapper.cpp qqmljsresourcefilemapper_p.h
        qqmljsscope.cpp qqmljsscope_p.h
        qqmljsscopesbyid_p.h
        qqmljsstreamwriter.cpp qqmljsstreamwriter_p.h
        qqmljstypedescriptionreader.cpp qqmljstypedescriptionreader_p.h
        qqmljstypereader.cpp qqmljstypereader_p.h
        qresourcerelocater.cpp qresourcerelocater_p.h
        qqmljsannotation_p.h qqmljsannotation.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
)

qt_internal_add_resource(QmlCompilerPrivate "builtins"
    PREFIX
        "/qt-project.org/qml/builtins"
    BASE
        "${CMAKE_CURRENT_SOURCE_DIR}/../imports/builtins/"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/../imports/builtins/builtins.qmltypes"
        "${CMAKE_CURRENT_SOURCE_DIR}/../imports/builtins/jsroot.qmltypes"
)

add_dependencies(QmlCompilerPrivate QmlDevToolsPrivate)
get_target_property(devtools_includes QmlDevToolsPrivate EXTRA_INCLUDE_DIRECTORIES)
target_include_directories(QmlCompilerPrivate PUBLIC ${devtools_includes})
target_compile_definitions(QmlCompilerPrivate PUBLIC QT_QMLDEVTOOLS_LIB)
