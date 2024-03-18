import QtQuick

ListView {
    width: 200
    height: 300
    spacing: 20
    orientation: ListView.Vertical

    header: Rectangle {
        x: (ListView.view.width - width) / 2
        color: 'tomato'
        width: 50
        height: 50
    }

    footer: Rectangle {
        x: (ListView.view.width - width) / 2
        color: 'lime'
        width: 50
        height: 50
    }

    model: 3
    delegate: Text {
        text: 'Foobar'
        horizontalAlignment: Text.AlignHCenter
        width: ListView.view.width
    }
}
