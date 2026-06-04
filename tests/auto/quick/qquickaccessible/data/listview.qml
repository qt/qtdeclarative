import QtQuick

ListView {
    model: listModel
    height: 300
    width: 100
    orientation: ListView.Vertical
    delegate: Text {
        height: 10
        width: 20
    }
}

