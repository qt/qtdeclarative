import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    QtObject {
        // QtQuick
        Accessible.name: "Foo"
        LayoutMirroring.enabled: true
        EnterKey.type: Qt.EnterKeyGo

        // QtQuick.Layouts
        Layout.minimumHeight: 3
        property bool stackLayout: StackLayout.isCurrentItem // Read-only

        // QtQuick.Templates
        ScrollBar.vertical: ScrollBar {}
        ScrollIndicator.vertical: ScrollIndicator {}
        SplitView.fillWidth: true
        StackView.visible: true
        property int swipeView: SwipeView.index // Read-only
        TextArea.flickable: TextArea {}
        ToolTip.delay: 50
        property bool swipeDelegate: SwipeDelegate.pressed // Read-only
    }
}
