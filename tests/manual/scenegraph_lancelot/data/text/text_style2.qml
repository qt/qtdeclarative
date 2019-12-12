import QtQuick 2.0

//vary font style

Item {
    width: 320
    height: 480

    Column {
        anchors.fill: parent
        Repeater {
            model: [Text.Normal, Text.Outline, Text.Raised, Text.Sunken]
            Text {
                renderType: Text.QtRendering
                width: parent.width
                wrapMode: Text.Wrap
                font.pointSize: 10
                style: modelData
                styleColor: "green"
                text: "The quick fox jumps in style " + modelData
            }
        }

        Repeater {
            model: [Text.Normal, Text.Outline, Text.Raised, Text.Sunken]
            Text {
                renderType: Text.NativeRendering
                width: parent.width
                wrapMode: Text.Wrap
                font.pointSize: 10
                style: modelData
                styleColor: "green"
                text: "The quick fox jumps in style " + modelData
            }
        }
    }
}
