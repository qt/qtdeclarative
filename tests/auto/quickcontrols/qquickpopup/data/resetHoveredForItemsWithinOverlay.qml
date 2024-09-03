import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 100
    height: 100
    property alias controlsPopup: _controlsPopup
    property alias blockInputPopup: _blockInputPopup
    Popup {
        id: _controlsPopup
        width: parent.width
        height: parent.height
        modal: true
        popupType: Popup.Item
        Control {
            id: controls
            anchors.fill: parent
            hoverEnabled: true
            contentItem: Text { text: "Test Control" }
        }
    }
    Popup {
        id: _blockInputPopup
        width: parent.width
        height: parent.height
        modal: true
        popupType: Popup.Item
    }
}
