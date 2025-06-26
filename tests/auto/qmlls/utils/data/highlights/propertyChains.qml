// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root

    // Test Case 1: known member of inner scope
    Item {
        id: test1
        property var obj: QtObject {
            property var foo: QtObject {
                property int bar: 42
            }
        }

        Component.onCompleted: {
            obj.foo.bar // Property, field, field
        }
    }

    // Test Case 2: ID inside the same component
    Item {
        id: test2

        Rectangle {
            id: noob
            width: 100
            color: "red"

            Component.onCompleted: {
                noob.color.toString() // ID, field, method
            }
        }
    }

    // Test Case 3: Known member of parent scope (Uncool)
    Item {
        id: test3

        property var someProperty: QtObject {
            property var someChild: 12
        }

        Item {
            property var xxxxx: someProperty // Property, unknown
        }
    }

    // Test Case 4: Unresolved lookup (should be unknown)
    Item {
        id: test4

        Component.onCompleted: {
            foo.bar // UNKNOWN, field,
        }
    }

    // Test Case 5: Outer ID lookup in nested component, good only if ComponentBehavior: Bound
    Item {
        id: outer

        Component {
            id: myComponent

            Rectangle {
                Component.onCompleted: {
                    outer.width.toFixed() // Outer ID, field, method
                }
            }
        }
    }

    // Test Case 6: Attached property chain
    Control {
        id: control
        
        Component.onCompleted: {
            console.log(
                control.contentItem.ListView.isCurrentItem,
                control.contentItem.mapToGlobal(0, 0)
            )
        }
    }
}
