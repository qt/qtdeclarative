// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

ListView {
    id: controlviewitem
    property string controlname: ""
    property variant controlvalue
    signal reset
    height: 40; width: 200; highlightRangeMode: ListView.StrictlyEnforceRange
    orientation: ListView.Horizontal; snapMode: ListView.SnapOneItem
    preferredHighlightBegin: 50; preferredHighlightEnd: 150;
    delegate: Rectangle { height: 40; width: 100; border.color: "black"
        Text { anchors.centerIn: parent; width: parent.width; text: model.name; elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter } }
    header: headfoot
    footer: headfoot
    Component {
        id: headfoot
        Rectangle {
            MouseArea { id: resetbutton; anchors.fill: parent; onClicked: { reset(); } }
            color: "lightgray"; height: 40; width: 50; border.color: "gray"
            Text { id: headertext; anchors.centerIn: parent; wrapMode: Text.WrapAnywhere
                rotation: -40; text: controlviewitem.controlname; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter }
        }
    }
}
