import QtQuick 2.15

Item {
    width: 640
    height: 480

    property string result: circle.name

    component MyCircle: Rectangle{
        id: rootMyCircle;
        property alias name: txtName.text
        property int age: 18
        height: 100; width: height; radius: height/2; anchors.centerIn: parent; color: "black";
        Column {
            anchors.centerIn: parent
            Text { id: txtName; color: "white"; text: "Foo" }
            Text { id: txtAge; color: "white"; text: age }
        }
    }

    MyCircle {
        id: circle
        property string something: ""
        name: "Bar"

        age: 21
        radius: 5
    }
}
