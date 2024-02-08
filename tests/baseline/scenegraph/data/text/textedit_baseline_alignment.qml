import QtQuick

Item {
    width: 320
    height: 160
    Item {
        width: 320
        height: 100
        Rectangle {
            id: alignItem
            width: parent.width
            height: 1
            color: "blue"
            anchors.bottom: parent.bottom
        }

        TextEdit {
            id: textItem
            textFormat: TextEdit.RichText
            font.family: "Arial"
            font.pixelSize: 50

            text: "a&#29340;&#2339;<span style=\"font-size: 25px\">a&#29340;&#2339;</span><span style=\"font-size: 10px\">a&#29340;&#2339;</span>"

            anchors.baseline: alignItem.top
            anchors.left: alignItem.left
        }
    }
}
