// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import "content"

pragma ComponentBehavior: Bound

//! [0]
Rectangle {
    id: root
    property int current: 0
    property bool increasing: true
    // Example index automation for convenience, disabled on click or tap
    SequentialAnimation {
        id: anim
        loops: -1
        running: true
        ScriptAction {
            script: if (root.increasing) {
                        root.current++;
                        if (root.current >= aModel.count -1) {
                            root.current = aModel.count - 1;
                            root.increasing = !root.increasing;
                        }
                    } else {
                        root.current--;
                        if (root.current <= 0) {
                            root.current = 0;
                            root.increasing = !root.increasing;
                        }
                    }
        }

        PauseAnimation {
            duration: 500
        }
    }
//! [0]
    MouseArea {
        id: ma
        z: 1
        anchors.fill: parent
        onClicked: function () {
            z = 1 - z;
            if (anim.running)
                anim.stop();
            else
                anim.restart();
        }
    }

    width: 320
    height: 480

    // This example shows the same model in three different ListView items,
    // with different highlight ranges. The highlight ranges are set by the
    // preferredHighlightBegin and preferredHighlightEnd properties in ListView.
    //
    // The first ListView does not set a highlight range, so its currentItem
    // can move freely within the visible area. If it moves outside the
    // visible area, the view is automatically scrolled to keep the current
    // item visible.
    //
    // The second ListView sets a highlight range which attempts to keep the
    // current item within the bounds of the range. However,
    // items will not scroll beyond the beginning or end of the view,
    // forcing the highlight to move outside the range at the ends.
    //
    // The third ListView sets the highlightRangeMode to StrictlyEnforceRange
    // and sets a range smaller than the height of an item.  This
    // forces the current item to change when the view is flicked,
    // since the highlight is unable to move.
    //
    // All ListViews bind their currentIndex to the root.current property.
    // The first ListView sets root.current whenever its currentIndex changes
    // due to keyboard interaction.
    // Flicking the third ListView with the mouse also changes root.current.
//! [1]
    ListView {
        id: list1
        height: 50
        width: parent.width
        model: PetsModel {
            id: aModel
        }
        delegate: petDelegate
        orientation: ListView.Horizontal
        highlight: Rectangle {
            color: "lightsteelblue"
        }
        currentIndex: root.current
        onCurrentIndexChanged: root.current = currentIndex
        focus: true
    }

    ListView {
        id: list2
        y: 160
        height: 50
        width: parent.width
        model: PetsModel { }
        delegate: petDelegate
        orientation: ListView.Horizontal
        highlight: Rectangle {
            color: "yellow"
        }
        currentIndex: root.current
        preferredHighlightBegin: 80
        preferredHighlightEnd: 220
        highlightRangeMode: ListView.ApplyRange
    }

    ListView {
        id: list3
        y: 320
        height: 50
        width: parent.width
        model: PetsModel {}
        delegate: petDelegate
        orientation: ListView.Horizontal
        highlight: Rectangle { color: "yellow" }
        currentIndex: root.current
        onCurrentIndexChanged: root.current = currentIndex
        preferredHighlightBegin: 125
        preferredHighlightEnd: 125
        highlightRangeMode: ListView.StrictlyEnforceRange
    }
//! [1]
    // The delegate for each list
    Component {
        id: petDelegate
        Item {
            id: petDelegateItem
            width: 160
            height: column.height

            required property int index
            required property string name
            required property string type
            required property int age

            Column {
                id: column
                Text {
                    text: 'Name: ' + petDelegateItem.name
                }
                Text {
                    text: 'Type: ' + petDelegateItem.type
                }
                Text {
                    text: 'Age: ' + petDelegateItem.age
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.current = parent.index
            }
        }
    }
//! [2]
}
//! [2]
