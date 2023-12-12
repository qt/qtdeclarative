import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Item {
    width: 360
    height: 280

    Rectangle {
        width: 200; height: 200; x: 100; y: 10
        color: th1.pressed ? "blue" : "lightblue"

        TapHandler {
            id: th1
            objectName: "th1"
        }

        Rectangle {
            width: 200; height: 200; x: 50; y: 50
            color: th2.pressed ? "steelblue" : "lightsteelblue"

            TapHandler {
                id: th2
                objectName: "th2"
            }
        }
    }

    Rectangle {
        width: 200; height: 200; x: 10; y: 50
        color: th3.pressed ? "goldenrod" : "beige"

        TapHandler {
            id: th3
            objectName: "th3"
        }
    }
}

