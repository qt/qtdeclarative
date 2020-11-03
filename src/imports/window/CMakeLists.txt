# special case skip regeneration

qt_internal_add_qml_module(quickwindow
    URI "QtQuick.Window"
    VERSION "${CMAKE_PROJECT_VERSION}"
    IMPORTS
        QtQuick/auto
    PAST_MAJOR_VERSIONS 2
    GENERATE_QMLTYPES
    INSTALL_QMLTYPES
)
