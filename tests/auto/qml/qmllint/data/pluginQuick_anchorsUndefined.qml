import QtQuick

Item {
    PluginQuickAnchorsBase {
        anchors.horizontalCenter: undefined
        anchors.verticalCenter: undefined
        anchors.baseline: undefined
        Component.onCompleted: anchors.bottom = undefined
    }
}
