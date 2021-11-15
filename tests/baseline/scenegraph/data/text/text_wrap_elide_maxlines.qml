import QtQuick 2.0

//test wrapping and elision when maximumLineCount is set

Item {
    width: 320
    height: 480
    Rectangle {
        id: text_area
        color: "light yellow"
        x: 50
        y: 0
        height: parent.height
        width: 150
    }
    Text {
        id: text_0000
        wrapMode: Text.WrapAnywhere
        text: "The quick brown fox jumps over the lazy dog."
        x: text_area.x
        y: text_area.y
        width: text_area.width
        maximumLineCount: 2
        elide: Text.ElideRight
        color: "red"
        font.family: "Arial"
        font.pixelSize: 22
    }
    Text {
        id: text_0001
        wrapMode: Text.Wrap
        text: text_0000.text
        anchors.top: text_0000.bottom
        anchors.left: text_0000.left
        width: text_0000.width
        maximumLineCount: text_0000.maximumLineCount
        elide: Text.ElideRight
        color: "blue"
        font.family: text_0000.font.family
        font.pixelSize: text_0000.font.pixelSize
    }
    Text {
        id: text_0002
        wrapMode: Text.WordWrap
        text: text_0000.text
        anchors.top: text_0001.bottom
        anchors.left: text_0000.left
        width: text_0000.width
        maximumLineCount: text_0000.maximumLineCount
        elide: Text.ElideRight
        color: "green"
        font.family: text_0000.font.family
        font.pixelSize: text_0000.font.pixelSize
    }
    Text {
        id: text_0003
        wrapMode: Text.WrapAnywhere
        text: "ABCDEFGHIJKL 1234567890123"
        anchors.top: text_0002.bottom
        anchors.left: text_0000.left
        width: 150
        maximumLineCount: 2
        elide: Text.ElideRight
        color: "red"
        font.family: text_0000.font.family
        font.pixelSize: text_0000.font.pixelSize
    }
    Text {
        id: text_0004
        wrapMode: Text.Wrap
        text: text_0003.text
        anchors.top: text_0003.bottom
        anchors.left: text_0000.left
        width: text_0000.width
        maximumLineCount: text_0000.maximumLineCount
        elide: Text.ElideRight
        color: "blue"
        font.family: text_0000.font.family
        font.pixelSize: text_0000.font.pixelSize
    }
    Text {
        id: text_0005
        wrapMode: Text.WordWrap
        text: text_0003.text
        anchors.top: text_0004.bottom
        anchors.left: text_0000.left
        width: text_0000.width
        maximumLineCount: text_0000.maximumLineCount
        elide: Text.ElideRight
        color: "green"
        font.family: text_0000.font.family
        font.pixelSize: text_0000.font.pixelSize
    }
    Text {
        id: text_0006
        wrapMode: Text.WrapAnywhere
        text: "The quick brown 1234567890123"
        anchors.top: text_0005.bottom
        anchors.left: text_0000.left
        width: 150
        maximumLineCount: 2
        elide: Text.ElideRight
        color: "red"
        font.family: text_0000.font.family
        font.pixelSize: text_0000.font.pixelSize
    }
    Text {
        id: text_0007
        wrapMode: Text.Wrap
        text: text_0006.text
        anchors.top: text_0006.bottom
        anchors.left: text_0000.left
        width: text_0000.width
        maximumLineCount: text_0000.maximumLineCount
        elide: Text.ElideRight
        color: "blue"
        font.family: text_0000.font.family
        font.pixelSize: text_0000.font.pixelSize
    }
    Text {
        id: text_0008
        wrapMode: Text.WordWrap
        text: text_0006.text
        anchors.top: text_0007.bottom
        anchors.left: text_0000.left
        width: text_0000.width
        maximumLineCount: text_0000.maximumLineCount
        elide: Text.ElideRight
        color: "green"
        font.family: text_0000.font.family
        font.pixelSize: text_0000.font.pixelSize
    }
}
