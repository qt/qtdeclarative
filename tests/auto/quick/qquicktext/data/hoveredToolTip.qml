import QtQuick

Item {
    width: 400
    height: 200

    Text {
        id: textItem
        objectName: "textItem"
        x: 10; y: 10
        width: 380
        // The tool tip is carried by the "title" attribute of the anchor.
        text: 'hover the <a href="#" title="the tool tip">word</a> here'
    }

    Rectangle {
        id: toolTip
        objectName: "toolTip"
        visible: textItem.hoveredToolTip.length > 0
        x: 100; y: 10
        width: label.implicitWidth + 12
        height: label.implicitHeight + 8
        color: "#ffffe1"
        border.color: "black"

        Text {
            id: label
            objectName: "toolTipLabel"
            anchors.centerIn: parent
            text: textItem.hoveredToolTip
        }
    }
}
