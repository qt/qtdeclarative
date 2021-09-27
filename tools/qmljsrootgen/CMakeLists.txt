qt_get_tool_target_name(target_name qmljsrootgen)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Global Object Metatypes Generator"
    TOOLS_TARGET Qml # special case
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
)
qt_internal_return_unless_building_tools()
