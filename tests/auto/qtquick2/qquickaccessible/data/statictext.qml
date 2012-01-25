import QtQuick 2.0

Item {
    width: 400
    height: 400

    Text {
        x: 100
        y: 20
        width: 200
        height: 50
        text : "Hello Accessibility"
    }

    Text {
        x: 100
        y: 40
        width: 100
        height: 40
        text : "Hello 2"
        Accessible.role: Accessible.StaticText
        Accessible.name: "The Hello 2 accessible text"
        Accessible.description: "A text description"
    }
}
