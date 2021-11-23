import QtQuick

Item {
    id: root
    y: z
    onXChanged: {
        root.y = 48
        ttt.font.pointSize = 22
    }
    z: 12

    Text {
        id: ttt
    }
}
