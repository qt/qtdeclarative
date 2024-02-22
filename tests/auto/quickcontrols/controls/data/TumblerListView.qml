// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ListView {
    implicitWidth: 60
    implicitHeight: 200
    snapMode: ListView.SnapToItem
    highlightRangeMode: ListView.StrictlyEnforceRange
    preferredHighlightBegin: height / 2 - (height / parent.visibleItemCount / 2)
    preferredHighlightEnd: height / 2 + (height / parent.visibleItemCount / 2)
    clip: true
    model: parent.model
    delegate: Text {
        objectName: text
        text: "Custom" + modelData
        opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
