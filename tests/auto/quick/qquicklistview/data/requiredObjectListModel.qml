import QtQuick 2.0

ListView {
    width: 100
    height: 100
    required model

    delegate: Rectangle {
        required color
        required property string name

        height: 25
        width: 100
    }
}
