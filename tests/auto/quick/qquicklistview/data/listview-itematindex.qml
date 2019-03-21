import QtQuick 2.0

ListView {
    width: 400
    height: 400
    focus: true
    model: 3

    delegate: Text {
        width: parent.width
        height: 10
        property int idx: index
        text: index
    }
}
