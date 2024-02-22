// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

PathView {
    id: pathView
    implicitWidth: 60
    implicitHeight: 200
    clip: true
    pathItemCount: parent.visibleItemCount + 1
    preferredHighlightBegin: 0.5
    preferredHighlightEnd: 0.5
    dragMargin: width / 2
    model: parent.model
    delegate: Text {
        objectName: text
        text: "Custom" + modelData
        opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    path: Path {
        startX: pathView.width / 2
        startY: -pathView.delegateHeight / 2
        PathLine {
            x: pathView.width / 2
            y: pathView.pathItemCount * pathView.delegateHeight - pathView.delegateHeight / 2
        }
    }

    property real delegateHeight: parent.availableHeight / parent.visibleItemCount
}
