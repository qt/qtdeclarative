// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.14
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: mainTableView
    property alias tableViewH: tableViewH
    property alias tableViewV: tableViewV
    property alias tableViewHV: tableViewHV

    property real delegateWidth: 30
    property real delegateHeight: 60

    Column {
        spacing: 10
        TableView {
            id: tableViewH
            objectName: "Hor"
            width: 600
            height: 100
            anchors.margins: 1
            clip: true
            delegate: tableViewDelegate
            syncView: mainTableView
            syncDirection: Qt.Horizontal
        }

        TableView {
            id: tableViewV
            objectName: "Ver"
            width: 600
            height: 100
            anchors.margins: 1
            clip: true
            delegate: tableViewDelegate
            syncView: mainTableView
            syncDirection: Qt.Vertical
        }

        TableView {
            id: tableViewHV
            objectName: "HorVer"
            width: 600
            height: 100
            anchors.margins: 1
            clip: true
            delegate: tableViewDelegate
            syncView: mainTableView
        }

        TableView {
            id: mainTableView
            objectName: "root"
            width: 600
            height: 100
            anchors.margins: 1
            clip: true
            delegate: tableViewDelegateMainView
            columnSpacing: 1
            rowSpacing: 1

            columnWidthProvider: function(c) { return 50 + c }
            rowHeightProvider: function(r) { return 25 + r }
        }
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            objectName: "tableViewDelegate"
            color: "lightblue"
            border.width: 1
            implicitWidth: 100
            implicitHeight: 100

            Text {
                anchors.centerIn: parent
                font.pixelSize: 10
                text: parent.TableView.view.objectName + "\n" + column + ", " + row
            }
        }
    }

    Component {
        id: tableViewDelegateMainView
        Rectangle {
            objectName: "tableViewDelegate"
            color: "lightgray"
            border.width: 1
            implicitWidth: delegateWidth
            implicitHeight: delegateHeight

            Text {
                anchors.centerIn: parent
                font.pixelSize: 10
                text: parent.TableView.view.objectName + "\n" + column + ", " + row
            }
        }
    }

}
