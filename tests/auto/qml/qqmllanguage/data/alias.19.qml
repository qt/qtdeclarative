import QtQuick

Item {
    id: myThing
    width: 1920

    MyRectangle {
        rectangle1AnchorsleftMargin: myThing.width / 2
        Component.onCompleted: myThing.height = rectangle1AnchorsleftMargin
    }
}
