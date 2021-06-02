# This qml module only exists to satisfy backward compatibility.
# The functionality was absorbed directly into QtQuick.

qt_internal_add_qml_module(quickwindow
    URI "QtQuick.Window"
    VERSION "${PROJECT_VERSION}"
    NO_PLUGIN_OPTIONAL
    PLUGIN_TARGET quickwindow
    CLASS_NAME QtQuick_WindowPlugin
    IMPORTS
        QtQuick/auto
    PAST_MAJOR_VERSIONS 2
)
