# Generated from moduleWithQmlSingleton.pro.

#####################################################################
## moduleWithQmlSingleton Generic Library:
#####################################################################

qt_add_cmake_library(moduleWithQmlSingleton
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/ModuleWithQmlSingleton"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:moduleWithQmlSingleton.pro:<TRUE>:
# COPIES = "importfiles" "subfiles"
# IMPORT_FILES = "qmldir" "MySingleton.qml" "MySingleton2.qml"
# TEMPLATE = "lib"
# subfiles.files = "internal/InternalType.qml"
# subfiles.path = "$$DESTDIR/internal"

# special case begin
file(COPY qmldir MySingleton.qml MySingleton2.qml
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/ModuleWithQmlSingleton"
)
# special case end

qt_autogen_tools_initial_setup(moduleWithQmlSingleton)
