/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

import QtQuick 2.15
import QtQml.Models 2.12

Rectangle {
    id: root
    width: 800
    height: 480

    property list<QtObject> myModel: [
        QtObject { property string name: "Item 0"; property bool selected: true },
        QtObject { property string name: "Item 1"; property bool selected: true },
        QtObject { property string name: "Item 2"; property bool selected: true },
        QtObject { property string name: "Item 3"; property bool selected: true },
        QtObject { property string name: "Item 4"; property bool selected: true },
        QtObject { property string name: "Item 5"; property bool selected: true },
        QtObject { property string name: "Item 6"; property bool selected: true },
        QtObject { property string name: "Item 7"; property bool selected: true },
        QtObject { property string name: "Item 8"; property bool selected: true },
        QtObject { property string name: "Item 9"; property bool selected: true },
        QtObject { property string name: "Press Enter here"; property bool selected: true }
    ]

    DelegateModel {
        objectName: "model"
        id: visualModel
        model: myModel
        filterOnGroup: "selected"

        groups: [
            DelegateModelGroup {
                name: "selected"
                includeByDefault: true
            }
        ]

        delegate: Rectangle {
            width: 180
            height: 180
            visible: DelegateModel.inSelected
            color: ListView.isCurrentItem ? "orange" : "yellow"
            Component.onCompleted: {
                DelegateModel.inPersistedItems = true
                DelegateModel.inSelected = Qt.binding(function() { return model.selected })
            }
        }
    }

    ListView {
        objectName: "list"
        anchors.fill: parent
        spacing: 180/15
        orientation: ListView.Horizontal
        model: visualModel
        focus: true
        currentIndex: 0
        preferredHighlightBegin: (width-180)/2
        preferredHighlightEnd: (width+180)/2
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 300
        highlightMoveVelocity: -1
        cacheBuffer: 0

        onCurrentIndexChanged: {
            if (currentIndex === 10) {
                myModel[6].selected = !myModel[6].selected
            }
        }
    }
}
