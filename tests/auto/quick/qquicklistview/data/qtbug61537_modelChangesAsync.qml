// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.0

Item {
    visible: true
    width: 640
    height: 480

    property ListView listView

    Loader {
        id: loader
        anchors.fill: parent
        asynchronous: true
        sourceComponent: comp

        onStatusChanged: {
            if (status == Loader.Ready) {
                // Assign the listview to the root prop late, so
                // that the c++ part doesn't start before everything is ready.
                listView = item.listView
            }
        }
    }

    Component {
        id: comp
        Item {
            property alias listView: listView

            ListView {
                id: listView

                model: ListModel {
                    id: listModel
                    ListElement { title: "one" }
                    ListElement { title: "two" }
                }

                anchors.fill: parent
                orientation: ListView.Horizontal

                delegate: Item {
                    id: delegateRoot
                    objectName: "delegate"

                    width: 200
                    height: 200

                    Component.onCompleted: {
                        if (index === listModel.count - 1) {
                            // Add a new item while the outer Loader is still incubating async. If the new model item
                            // incubates using e.g QQmlIncubator::AsynchronousIfNested, it will also be loaded async, which
                            // is not currently supported (the item will not be added to the listview, or end up the wrong
                            // position, depending on its index and the current state of the refill/layout logic in
                            // QQuickListView).
                            // We add the new item here at the last delegates Component.onCompleted to hit the point in time
                            // when the listview is not expecting any more async items. In that case, the item will only be
                            // added to the list of visible items if incubated synchronously, which gives us something we
                            // can test for in the auto-test.
                            listModel.insert(0, {title: "zero"});
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: index
                        }
                    }
                }
            }
        }
    }
}
