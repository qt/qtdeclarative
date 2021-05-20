import QtQuick

Item {
    QtObject { id: object }
    Item { id: item }
    Rectangle { id: rectangle }

    property QtObject objectAsObject: object as QtObject
    property QtObject objectAsItem: object as Item
    property QtObject objectAsRectangle: object as Rectangle

    property QtObject itemAsObject: item as QtObject
    property QtObject itemAsItem: item as Item
    property QtObject itemAsRectangle: item as Rectangle

    property QtObject rectangleAsObject: rectangle as QtObject
    property QtObject rectangleAsItem: rectangle as Item
    property QtObject rectangleAsRectangle: rectangle as Rectangle
}
