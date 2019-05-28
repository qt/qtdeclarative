/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

Item {
    width: 640
    height: 450

    property alias tableView: mainTableView
    property alias tableViewH: tableViewH
    property alias tableViewV: tableViewV
    property alias tableViewHV: tableViewHV

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
            delegate: tableViewDelegate
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
            color: "lightgray"
            border.width: 1
            implicitWidth: 30
            implicitHeight: 60

            Text {
                anchors.centerIn: parent
                font.pixelSize: 10
                text: parent.TableView.view.objectName + "\n" + column + ", " + row
            }
        }
    }

}
