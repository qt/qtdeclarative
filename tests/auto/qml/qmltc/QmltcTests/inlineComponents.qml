// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick as MyQtQuick

Item {
    id: root

    component MyComponent: Item {
        id: root
        property string myX: "SharedX"
        property string myProperty: "check literal binding"
        property int specialNumber: 123

        Item {
            id: notInDocumentRoot
            property string myZ: "SharedZ"
            property alias aliasToMyX: root.myX // alias to own stuff
        }
    }

    //check that inline components can be used in properties/signals
    property MyComponent myMyComponent: MyComponent {}
    property QtObject myMyComponentAsQtObject: MyComponent {}
    // signal myMyComponentSignal(arg1: MyComponent) // todo: investigate
    signal myMyComponentSignal(arg1: QtObject) // todo: investigate
    property int signalTriggered: 0
    onMyMyComponentSignal: (comp) => {
        signalTriggered = comp.specialNumber
    }

    component A : Item {
        property string data
        property B b // NB: if "b: B {}", it should be a recursion error
    }

    component B : Item {
        property A a
    }

    component APlus: A {
        property var hello
    }
    component APlusPlus: APlus {
        property var hellohello
    }

    property MyComponent myMyComponentComponent: MyComponent {
        id: myMyComponentComponent
        property string myY: "NotSharedY1"
        myX: "MyComponent1"
    }

    property MyComponent myMyComponentComponent2: MyComponent {
        id: myMyComponentComponent2
        property string myY2: "NotSharedY2"
        myX: "MyComponent2"
    }

    property A myAComponent: A {
        id: myAComponent
        data: "Hello From Outside!"
        b: B {
            id: myBComponent
            a: A {
                id: innerA
                data: "Hello From Inside!"
            }
        }
    }

    property var normalType: Item {
        id: normalType

        property var normalProperty

        Item {
            id: innerNormalType

            property var innerNormalProperty
        }
    }

    property string conflicting: "inner"
    component ConflictingComponent: Item {
        Item {
            // should not collide with the document root called id.
            id: root

            // should not collide with same property in document root
            property string conflicting: "outer"
        }

        required property string myName
        property string output: myName + ": " + root.conflicting

    }
    property string output: root.conflicting
    property var tom: ConflictingComponent {
        myName: "Tom"
    }
    property var jerry: ConflictingComponent {
        myName: "Jerry"
    }

    component EmptyComponent: QtObject {}

    property var empty: EmptyComponent {
        objectName: "EmptyComponentObject"
    }

    component Rectangle: Item {
        property string myData: "Not from QtQuick.Rectangle"
    }
    property Rectangle inlineComponentFoundBeforeOtherImports: Rectangle {}

    property var inlineComponentOrder: IC2 {
        // anchors.centerIn: parent
    }

    component IC2: IC1 {}
    component IC0: MyQtQuick.Rectangle {
        height: 200
        width: 200
        color: "blue"
    }
    component IC1: IC0 {}

    component ComponentForList : QtObject {
        property string name
        property int age
    }

    property list<ComponentForList> componentList: [
        ComponentForList { name: "Alfred Ill"; age: 65 },
        ComponentForList { name: "Claire Zachanassian"; age: 62}
    ]

    property list<QtObject> testList: [
        QtObject {},
        QtObject {},
        ComponentForList { name: "Claire Zachanassian"; age: 62}
    ]
}
