add_library(QmlLintPluginQuick STATIC
    quicklintplugin.h
    quicklintplugin.cpp
)
target_link_libraries(QmlLintPluginQuick PRIVATE Qt::QmlCompilerPrivate)
qt_autogen_tools_initial_setup(QmlLintPluginQuick)
target_compile_definitions(QmlLintPluginQuick PRIVATE QT_STATICPLUGIN)

