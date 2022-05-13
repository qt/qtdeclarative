// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Window 2.3

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        width: 600
        height: 400
        delegate: tableViewDelegate
    }

    Component {
        id: tableViewDelegate
        Rectangle {
            id: rect
            required property string position
            required property bool unset
            required property bool hasModelChildren
            required property QtObject model
            Text {text: rect.position}
            implicitWidth: 100
            implicitHeight: 100
            Component.onCompleted: () => {if (rect.position === "R1:C1" && rect.model.hasModelChildren == rect.hasModelChildren)  console.info("success")}
        }
    }

}
