import QtQuick

Item {
    width: 400
    height: 200

    TextEdit {
        id: textEdit
        objectName: "textEdit"
        x: 10; y: 10
        width: 380
        font.pixelSize: 20
        textFormat: TextEdit.RichText
        // The tool tip is carried by the "title" attribute of the anchor.
        text: "Test <a href='http://example.com/' title='the tool tip'>link</a>"
    }

    Rectangle {
        id: toolTip
        objectName: "toolTip"
        visible: textEdit.hoveredToolTip.length > 0
        x: 60; y: 10
        width: label.implicitWidth + 12
        height: label.implicitHeight + 8
        color: "#ffffe1"
        border.color: "black"

        Text {
            id: label
            objectName: "toolTipLabel"
            anchors.centerIn: parent
            text: textEdit.hoveredToolTip
        }
    }
}
