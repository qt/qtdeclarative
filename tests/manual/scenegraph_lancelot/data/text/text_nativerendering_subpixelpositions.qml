import QtQuick 2.0

//vary font style, native rendering at non-integer offsets

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
            text: "The quick fox jumps in style " + modelData
        }
    }

    Repeater {
        model: [Text.Normal, Text.Outline, Text.Raised, Text.Sunken]
        Text {
            y: 100.5 + 20 * index
            clip: true
            renderType: Text.NativeRendering
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
            y: 200.5 + 20 * index
            x: 0.5
            clip: true
            renderType: Text.NativeRendering
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
            y: 300.5 + 20 * index
            x: 0.5
            clip: true
            renderType: Text.NativeRendering
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
        Rectangle {
            y: 400.5 + 20 * index
            x: 0.5
            width: topLevel.width
            height: topLevel.height
            clip: true
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
