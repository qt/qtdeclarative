set(DEFAULT_SOURCE_PATH resources/default.qml)
qt_add_resources(${PROJECT_NAME} "Resources"
    PREFIX "/"
    FILES
        ${DEFAULT_SOURCE_PATH}
        resources/logo.png
)
add_definitions(-DDEFAULT_SOURCE_PATH=":/${DEFAULT_SOURCE_PATH}")
