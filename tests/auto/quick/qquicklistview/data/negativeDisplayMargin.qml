// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.3

ListView {
    id: list
    width: 400
    height: 600
    model: ListModel {
        ListElement { kind: "Bought" }
        ListElement { kind: "Available To Buy" }
    }

    delegate: ListView {
        id: innerList
        objectName: "innerList"
        height: count * lineHeight
        width: list.width
        interactive: false
        property int count: 50
        model: count
        property int createdItems: 0
        property int destroyedItems: 0
        property int lineHeight: 85

        delegate: Item {
            objectName: "delegate"
            width: innerList.width
            height: innerList.lineHeight
            Rectangle {
                width: parent.width - 20
                height: parent.height - 20
                anchors.centerIn: parent
                color: Math.random() * 2 > 1 ? "green" : "yellow";
                Text {
                    text: index
                }
                Component.onCompleted: createdItems++
                Component.onDestruction: destroyedItems++
            }
        }

        displayMarginBeginning: 0
        displayMarginEnd: -height

        function updatedDelegateCreationRange() {
            if (list.contentY + list.height <= innerList.y) {
                // Not visible
                innerList.displayMarginBeginning = 0
                innerList.displayMarginEnd = -innerList.height
            } else if (innerList.y + innerList.height <= list.contentY) {
                // Not visible
                innerList.displayMarginBeginning = -innerList.height
                innerList.displayMarginEnd = 0
            } else {
                innerList.displayMarginBeginning = -Math.max(list.contentY - innerList.y, 0)
                innerList.displayMarginEnd = -Math.max(innerList.height - list.height - list.contentY + innerList.y, 0)
            }
        }

        Component.onCompleted: updatedDelegateCreationRange();
        onHeightChanged: updatedDelegateCreationRange();
        Connections {
            target: list
            function onContentYChanged() { updatedDelegateCreationRange(); }
            function onHeightChanged() { updatedDelegateCreationRange(); }
        }
    }

    section.property: "kind"
    section.delegate: Text {
        height: 40
        font.pixelSize: 30
        text: section
    }
}
