import QtQuick 2.0

Item {
    Text {
        id: foo
        text: "Foo" // Specific property we're targeting in one element
        x: 5 // Property we're targeting regardless of type
        y: x + 5 // Property used in a binding
        font.bold: true // Not targeted
    }
    x: 5  // Property we're targeting regardless of type

    ListModel { id: listModel }

    ListView { // Entire type covered
        model: listModel
        height: 50
    }

    Component.onCompleted: {
        console.log(foo.x); // Reading property from another component
        foo.x = 30; // Writing property from another component
    }
}
