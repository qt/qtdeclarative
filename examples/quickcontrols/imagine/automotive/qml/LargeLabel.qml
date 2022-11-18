// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

// This container and the transform on the Label are
// necessary to get precise bounding rect of the text for layouting reasons,
// since some of the labels' font sizes can get quite large.
Item {
    id: root
    implicitHeight: labelTextMetrics.tightBoundingRect.height
    implicitWidth: label.implicitWidth

    property alias text: label.text
    property alias font: label.font
    property alias horizontalAlignment: label.horizontalAlignment
    property alias verticalAlignment: label.verticalAlignment
    property bool glowEnabled: true
    property color color: colorBright

    Label {
        id: label
        anchors.baseline: root.baseline
        color: root.color

        TextMetrics {
            id: labelTextMetrics
            text: label.text
            font: label.font
        }

        transform: Translate {
            y: -labelTextMetrics.tightBoundingRect.y
        }
    }
}
