import QtQuick 2.0

//vary font style, native rendering without antialiasing

Item {
    id: topLevel
    width: 320
    height: 580

    Repeater {
        model: [Text.Normal, Text.Outline, Text.Raised, Text.Sunken]
        Text {
            y: 20 * index
            clip: true
            renderType: Text.NativeRendering
            width: parent.width
            wrapMode: Text.Wrap
            font.pointSize: 10
            style: modelData
            styleColor: "green"
            antialiasing: false
            text: "The quick fox jumps in style " + modelData
        }
    }
}
