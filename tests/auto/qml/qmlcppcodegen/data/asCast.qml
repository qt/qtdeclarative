// Copyright (C) 2021 The Qt Company Ltd.

pragma Strict
import QtQuick

Item {
    QtObject { id: object }
    Item { id: item }
    Rectangle { id: rectangle }
    Dummy { id: dummy }

    property QtObject objectAsObject: object as QtObject
    property QtObject objectAsItem: object as Item
    property QtObject objectAsRectangle: object as Rectangle
    property QtObject objectAsDummy: object as Dummy

    property QtObject itemAsObject: item as QtObject
    property QtObject itemAsItem: item as Item
    property QtObject itemAsRectangle: item as Rectangle
    property QtObject itemAsDummy: item as Dummy

    property QtObject rectangleAsObject: rectangle as QtObject
    property QtObject rectangleAsItem: rectangle as Item
    property QtObject rectangleAsRectangle: rectangle as Rectangle
    property QtObject rectangleAsDummy: rectangle as Dummy

    property QtObject dummyAsObject: dummy as QtObject
    property QtObject dummyAsItem: dummy as Item
    property QtObject dummyAsRectangle: dummy as Rectangle
    property QtObject dummyAsDummy: dummy as Dummy
}
