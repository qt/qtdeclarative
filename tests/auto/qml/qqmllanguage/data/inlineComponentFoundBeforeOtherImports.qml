import QtQuick 2.12

Item
{
    component Rectangle: Item {
        Component.onCompleted: console.info("Created")
    }
    Rectangle {}
}
