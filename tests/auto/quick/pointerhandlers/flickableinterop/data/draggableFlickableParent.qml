// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// A DragHandler on the parent Item of a Flickable (ListView).
// When dragging starts within the ListView, the Flickable should scroll,
// and the DragHandler should NOT activate (QTBUG-75074, QTBUG-79238).
Item {
    id: root
    width: 400
    height: 400

    DragHandler { }

    ListView {
        anchors.fill: parent
        model: 200
        delegate: Rectangle {
            required property int modelData
            width: ListView.view.width
            height: 30
            color: modelData % 2 ? "lightgrey" : "grey"
            Text { text: parent.modelData }
        }
    }
}
