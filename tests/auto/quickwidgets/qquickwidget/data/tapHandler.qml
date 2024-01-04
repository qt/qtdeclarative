import QtQuick

Rectangle {
    width: 100
    height: 100
    color: th.pressed ? "steelblue" : "beige"

    TapHandler {
        id: th
    }
}
