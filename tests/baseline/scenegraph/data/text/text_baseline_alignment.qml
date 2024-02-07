import QtQuick

Item {
    width: 320
    height: 300
    Column {
        anchors.fill: parent
        Repeater {
            model: [ Text.RichText, Text.StyledText ]
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

                Text {
                    id: textItem
                    textFormat: modelData
                    font.family: "Arial"
                    font.pixelSize: 50

                    text: textFormat == Text.RichText
                           ? "a&#29340;&#2339;<span style=\"font-size: 25px\">a&#29340;&#2339;</span><span style=\"font-size: 10px\">a&#29340;&#2339;</span>"
                           : "<font size=\"3\">a&#29340;&#2339;</font><font size=\"3\">a&#29340;&#2339;</font><font size=\"1\">a&#29340;&#2339;</font>"

                    anchors.baseline: alignItem.top
                    anchors.left: alignItem.left
                }
            }
        }
    }
}
