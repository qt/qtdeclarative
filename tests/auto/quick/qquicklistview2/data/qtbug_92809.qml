// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

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
