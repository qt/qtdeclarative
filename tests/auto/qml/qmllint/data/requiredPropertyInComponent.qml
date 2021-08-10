import QtQuick 2.0

Row {
    // explicit component
    Component {
        id: foo
        Item {
            required property int i
        }
    }

    // implicit component, plain property
    property Component com: Item {}

    Repeater {
        model: 3
        // implicit component, default property
        Text {
            required property int index
            height: 40
            color: "black"
            text: "I'm item " + index
        }
    }
}
