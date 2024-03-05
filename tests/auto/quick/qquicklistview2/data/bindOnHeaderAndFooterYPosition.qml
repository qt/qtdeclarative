import QtQuick

ListView {
    width: 300
    height: 200
    spacing: 20
    orientation: ListView.Horizontal

    header: Rectangle {
        y: (ListView.view.height - height) / 2
        color: 'tomato'
        width: 50
        height: 50
    }

    footer: Rectangle {
        y: (ListView.view.height - height) / 2
        color: 'lime'
        width: 50
        height: 50
    }

    model: 3
    delegate: Text {
        text: 'Foobar'
        verticalAlignment: Text.AlignVCenter
        height: ListView.view.height
    }
}
