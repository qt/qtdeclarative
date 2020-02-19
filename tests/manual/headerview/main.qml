/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQml.Models 2.15
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Shapes 1.15
import QtQuick.Window 2.15
import Qt.labs.qmlmodels 1.0
import TestTableModelWithHeader 0.1

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("HeaderView Test")

    TestTableModelWithHeader {
        id: tableModel
        rowCount: 50
        columnCount: 80
    }

    TableView {
        id: tableView
        anchors.top: parent.top
        anchors.topMargin: horizontalHeader.height + rowSpacing
        anchors.left: parent.left
        anchors.leftMargin: verticalHeader.width + columnSpacing
        model: tableModel
        rightMargin: 100
        bottomMargin: 100
        columnSpacing: 4
        rowSpacing: 4
        syncDirection: Qt.Vertical | Qt.Horizontal
        implicitWidth: parent.width + columnSpacing
        implicitHeight: parent.height + rowSpacing
        clip: true
        delegate: Rectangle {
            implicitWidth: 150
            implicitHeight: 50
            color: "#e6ecf5"

            CheckBox {
                anchors.fill: parent
                text: model.display
                checked: model.edit
                leftPadding: 12
                onClicked: model.edit = checked
            }
        }
    }

    HorizontalHeaderView {
        id: horizontalHeader
        objectName: "horizontalHeader"
        anchors.top: parent.top
        anchors.left: tableView.left
        syncView: tableView
        clip: true
    }

    VerticalHeaderView {
        id: verticalHeader
        objectName: "verticalHeader"
        anchors.top: tableView.top
        syncView: tableView
        clip: true
    }

    ToolButton {
        width: verticalHeader.width
        height: horizontalHeader.height
        onClicked: {
            horizontalHeader.contentX = 0
            verticalHeader.contentY = 0
        }
    }
}
