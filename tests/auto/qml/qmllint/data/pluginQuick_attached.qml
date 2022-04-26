import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
QtObject {
    // QtQuick
    Accessible.name: "Foo"
    LayoutMirroring.enabled: true
    EnterKey.type: Qt.EnterKeyGo

    // QtQuick.Layouts
    Layout.minimumHeight: 3

    // QtQuick.Templates
    ScrollBar.vertical: ScrollBar {}
    ScrollIndicator.vertical: ScrollIndicator {}
    SplitView.fillWidth: true
    StackView.visible: true
    ToolTip.delay: 50
}
