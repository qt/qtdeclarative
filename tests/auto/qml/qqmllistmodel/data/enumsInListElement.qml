import QtQuick

ListView {
    width: 180
    height: 200
    model: Model {}
    delegate: Text { text: choose }
}
