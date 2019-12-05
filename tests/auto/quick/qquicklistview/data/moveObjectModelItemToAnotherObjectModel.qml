/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.14
import QtQml.Models 2.14

Item {
    id: root
    width: 400
    height: 400

    readonly property int rectCount: 3
    property var rectColors: ["red", "green", "blue"]

    property alias listView1: listView1
    property alias listView2: listView2

    function moveRedRectToModel2() {
        var appItem = objectModel1.get(0)
        objectModel1.remove(0, 1)
        objectModel2.insert(0, appItem)
    }

    function moveRedRectToModel1() {
        var appItem = objectModel2.get(0)
        objectModel2.remove(0, 1)
        objectModel1.insert(0, appItem)
    }

    ObjectModel {
        id: objectModel1
        objectName: "objectModel1"

        Component.onCompleted: {
            for (var i = 0; i < root.rectCount; i++) {
                var outerRect = rectComponent.createObject(null, {
                    "objectName": root.rectColors[i] + "Rect",
                    "color": root.rectColors[i]
                })
                objectModel1.append(outerRect)
            }
        }
    }

    ObjectModel {
        id: objectModel2
        objectName: "objectModel2"
    }

    ListView {
        id: listView1
        objectName: "listView1"
        anchors.left: parent.left
        anchors.top: parent.top
        height: 100
        width: 100
        anchors.margins: 20
        clip: true
        cacheBuffer: 0
        model: objectModel1
        orientation: ListView.Horizontal
        spacing: 20

        Component.onCompleted: contentItem.objectName = "listView1ContentItem"
    }

    ListView {
        id: listView2
        objectName: "listView2"
        anchors.right: parent.right
        anchors.top: parent.top
        height: 100
        width: 100
        anchors.margins: 20
        clip: true
        cacheBuffer: 0
        model: objectModel2
        orientation: ListView.Horizontal
        spacing: 20

        Component.onCompleted: contentItem.objectName = "listView2ContentItem"
    }

    Component {
        id: rectComponent

        Rectangle {
            height: 100
            width: 100
            opacity: 0.2
        }
    }
}
