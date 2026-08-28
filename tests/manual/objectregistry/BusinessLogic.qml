// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// This file simulates hand-written QML business logic part of the application.
// The UI elements are accessed via ObjectRegistry references.

import QtQuick
import QtQml.DesignSupport

Item {
    id: root

    ListModel {
        id: contactsModel
        ListElement {
            name: "Entry A"
            number: "1"
        }
        ListElement {
            name: "Entry B"
            number: "2"
        }
        ListElement {
            name: "Entry C"
            number: "3"
        }
    }

    ObjectRegistryRef {
        id: listViewRef
        key: "ListView"

        onObjectChanged: (object)=> {
            object.model = contactsModel

            // Manual check that object property values are all initialized
            console.log("listView - x: " + object.x)
            console.log("listView - width: " + object.width)
            console.log("listView - height: " + object.height)
            console.log("listView - myValue: " + object.myValue)
            console.log("listView - parent: " + object.parent)
        }
    }

    ObjectRegistryRef {
        id: firstRectRef
        key: "FirstRect"
    }

    ObjectRegistryRef {
        id: secondRectRef
        key: "SecondRect"
    }

    ObjectRegistryRef {
        id: swapButtonTextRef
        key: "SwapButtonText"
    }

    ObjectRegistryRef {
        id: buttonRef
        key: "MyButton"

        Connections {
            target: buttonRef.object
            function onClicked(mouse) {
                let tempColor = firstRectRef.object.color.toString()
                firstRectRef.object.color = secondRectRef.object.color
                secondRectRef.object.color = tempColor

                // This will stop working after "CppDeleteObjButton" is deleted due to duplicate
                // registration with one of the registrations deleted
                if (swapButtonTextRef.object)
                    swapButtonTextRef.object.color = firstRectRef.object.color
            }
        }
    }

    MultiObjectRegistryRef {
        id: repeaterRef
        key: "RepeatedRect"

        onObjectAdded: (object)=> {
            object.clicked.connect(function (mouse) { clickedHandler(object, mouse); })
        }

        function clickedHandler(object, mouse) {
            indexTextRef.object.text = "Clicked index: " + object.repeaterIndex
        }

        ObjectRegistryRef {
            id: indexTextRef
            key: "IndexText"
        }
    }

    MultiObjectRegistryRef {
        id: listRef
        key: "ListDelegate"

        onObjectAdded: (object)=> {
            console.log("object added to list=" + object.ListView.view
                        + ", count: " + listRef.objects.length)
            object.clicked.connect(function () { clickedHandler(object); })
        }

        onObjectRemoved: (object)=> {
            // Note: object will be null if removal happens because of object destruction
            console.log("object removed from list=" + object + ", count: " + listRef.objects.length)
        }

        function clickedHandler(object) {
            listLabelRef.object.text = "Clicked delegate: " + object.text
        }

        ObjectRegistryRef {
            id: listLabelRef
            key: "List Label"

            onObjectChanged: (object)=> {
                // Manual check that object property values are all initialized
                console.log("listLabel - x: " + object.x)
                console.log("listLabel - text: " + object.text)
                console.log("listLabel - font.bold: " + object.font.bold)
                console.log("listLabel - font.italic: " + object.font.italic)
                console.log("listLabel - font.family: " + object.font.family)
                console.log("listLabel - parent: " + object.parent)
            }
        }
    }

    ObjectRegistryRef {
        id: addRef
        key: "AddButton"

        property int nextNumber: 4

        Connections {
            target: addRef.object
            function onClicked(mouse) {
                contactsModel.append({name:"New Entry", number: "" + addRef.nextNumber++})
            }
        }
    }

    ObjectRegistryRef {
        id: removeRef
        key: "RemoveButton"

        Connections {
            target: removeRef.object
            function onClicked(mouse) {
                contactsModel.remove(contactsModel.count - 1, 1)
            }
        }
    }

    // Test registering two objects with one key warning
    ObjectRegistryRef {
        key: "DoubleRegister"
    }
}
