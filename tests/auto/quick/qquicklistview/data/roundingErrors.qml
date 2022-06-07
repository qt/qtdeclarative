// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick 2.0
import QtQml.Models 2.1

ListView {
    id: listview

    width: 600
    height: 200

    spacing: 8
    orientation: ListView.Horizontal

    Component.onCompleted: {
        var colors = ["blue", "green", "red", "yellow", "orange", "purple", "cyan",
                      "magenta", "chartreuse", "aquamarine", "indigo", "lightsteelblue",
                      "violet", "grey", "springgreen", "salmon", "blanchedalmond",
                      "forestgreen", "pink", "navy", "goldenrod", "crimson", "teal" ]
        for (var i = 0; i < 100; ++i)
            colorModel.append( { nid: i,  color: colors[i%colors.length] } )
    }

    model: DelegateModel {
        id: visualModel

        model: ListModel {
            id: colorModel
        }

        delegate: MouseArea {
            id: delegateRoot
            objectName: model.nid

            width: 107.35
            height: 63.35

            drag.target: icon

            Rectangle{
                id: icon
                width: delegateRoot.width
                height: delegateRoot.height
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                color: model.color

                Drag.active: delegateRoot.drag.active
                Drag.source: delegateRoot
                Drag.hotSpot.x: 36
                Drag.hotSpot.y: 36

                Text {
                    id: text
                    anchors.fill: parent
                    font.pointSize: 40
                    text: model.nid
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                }

                states: [
                    State {
                        when: icon.Drag.active
                        ParentChange {
                            target: icon
                            parent: delegateRoot.ListView.view
                        }

                        AnchorChanges {
                            target: icon
                            anchors.horizontalCenter: undefined
                            anchors.verticalCenter: undefined
                        }
                    }
                ]
            }
        }
    }

    DropArea {
        anchors.fill: parent
        onPositionChanged: (drag) => {
            var to = listview.indexAt(drag.x + listview.contentX, 0)
            if (to !== -1) {
                var from = drag.source.DelegateModel.itemsIndex
                if (from !== to)
                    visualModel.items.move(from, to)
                drag.accept()
            }
        }
    }
}
