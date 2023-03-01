import QtQuick 2.0

ListView {
    id: listView
    width: 400
    height: 400
    focus: true
    model: 3

    delegate: Text {
        width: listView.width
        height: 10
        property int idx: index
        text: index
    }
}
