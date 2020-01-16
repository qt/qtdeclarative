/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.14
import QtQuick.Window 2.3
import QtQml.Models 2.2
import TestTableModel 0.1
import QtQuick.Controls 2.5

Window {
    id: window
    width: 640
    height: 480
    visible: true

    property int selectedX: -1
    property int selectedY: -1

    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        color: "darkgray"

        Column {
            id: menu
            x: 2
            y: 2

            Row {
                spacing: 1
                Button {
                    text: "Add row"
                    onClicked: tableView.model.insertRows(selectedY, 1)
                }
                Button {
                    text: "Remove row"
                    onClicked: tableView.model.removeRows(selectedY, 1)
                }
                Button {
                    text: "Add column"
                    onClicked: tableView.model.insertColumns(selectedX, 1)
                }
                Button {
                    text: "Remove column"
                    onClicked: tableView.model.removeColumns(selectedX, 1)
                }
            }

            Row {
                spacing: 1
                Button {
                    text: "fast-flick<br>center table"
                    onClicked: {
                        tableView.contentX += tableView.width * 1.2
                    }
                }
                Button {
                    text: "flick to end<br>center table"
                    onClicked: {
                        tableView.contentX = tableView.contentWidth - tableView.width
                    }
                }
                Button {
                    text: "fast-flick<br>headers"
                    onClicked: {
                        leftHeader.contentY += 1000
                        topHeader.contentX += 1000
                    }
                }
                Button {
                    text: "set/unset<br>master bindings"
                    onClicked: {
                        leftHeader.syncView = leftHeader.syncView ? null : tableView
                        topHeader.syncView = topHeader.syncView ? null : tableView
                    }
                }
                Button {
                    text: "inc space"
                    onClicked: {
                        tableView.columnSpacing += 1
                        tableView.rowSpacing += 1
                    }
                }
            }
            Text {
                text: "Selected: x:" + selectedX + ", y:" + selectedY
            }
        }

        TableView {
            id: topHeader
            objectName: "topHeader"
            anchors.left: tableView.left
            width: tableView.width
            anchors.top: menu.bottom
            height: 30
            clip: true
            ScrollBar.horizontal: ScrollBar {}

            model: TestTableModel {
                rowCount: 1
                columnCount: 200
            }

            delegate: Rectangle {
                implicitHeight: topHeader.height
                implicitWidth: 20
                color: "lightgray"
                Text { text: column }
            }

            columnSpacing: 1
            rowSpacing: 1

            syncDirection: Qt.Horizontal
        }

        TableView {
            id: leftHeader
            objectName: "leftHeader"
            anchors.left: parent.left
            anchors.top: tableView.top
            height: tableView.height
            width: 30
            clip: true
            ScrollBar.vertical: ScrollBar {}

            model: TestTableModel {
                rowCount: 200
                columnCount: 1
            }

            delegate: Rectangle {
                implicitHeight: 50
                implicitWidth: leftHeader.width
                color: "lightgray"
                Text { text: row }
            }

            columnSpacing: 1
            rowSpacing: 1

            syncDirection: Qt.Vertical
        }

        TableView {
            id: tableView
            objectName: "tableview"
            anchors.left: leftHeader.right
            anchors.right: parent.right
            anchors.top: topHeader.bottom
            anchors.bottom: parent.bottom
            width: 200
            clip: true
            columnWidthProvider: function(c) {
                if (c > 30)
                    return 100
                else
                    return 200;
            }
            ScrollBar.horizontal: ScrollBar {}
            ScrollBar.vertical: ScrollBar {}

            model: TestTableModel {
                rowCount: 200
                columnCount: 60
            }
            delegate: tableViewDelegate
            columnSpacing: 1
            rowSpacing: 1
        }

        Component {
            id: tableViewDelegate
            Rectangle {
                id: delegate
                implicitWidth: 100
                implicitHeight: 50
                color: display
                border.width: row === selectedY && column == selectedX ? 2 : 0
                border.color: "darkgreen"

                Text {
                    anchors.fill: parent
                    text: column + ", " + row

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            selectedX = column
                            selectedY = row
                        }
                    }
                }
            }
        }

    }

}
