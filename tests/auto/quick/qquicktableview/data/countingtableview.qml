// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    id: root
    width: 640
    height: 450

    property alias tableView: tableView

    // currentDelegateCount is the number of currently visible items
    property int currentDelegateCount: 0
    // maxDelegateCount is the largest number of items that has ever been visible at the same time
    property int maxDelegateCount: 0
    // delegatesCreatedCount is the number of items created during the lifetime of the test
    property int delegatesCreatedCount: 0

    property real delegateWidth: 100
    property real delegateHeight: 50

    TableView {
        id: tableView
        width: 600
        height: 400
        anchors.margins: 1
        clip: true
        delegate: tableViewDelegate
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            implicitWidth: delegateWidth
            implicitHeight: delegateHeight
            color: "lightgray"
            border.width: 1

            property int pooledCount: 0
            property int reusedCount: 0
            TableView.onPooled: pooledCount++;
            TableView.onReused: reusedCount++;

            Text {
                anchors.centerIn: parent
                text: column
            }
            Component.onCompleted: {
                delegatesCreatedCount++;
                currentDelegateCount++;
                maxDelegateCount = Math.max(maxDelegateCount, currentDelegateCount);
            }
            Component.onDestruction: {
                currentDelegateCount--;
            }
        }
    }

}
