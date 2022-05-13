// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window

Window {
    // Different delegate positions and widths and window background colors
    // can cause some unwanted "rogue pixels", so an easy way to get it perfect
    // is to mess with the width.
    width: 814
    height: 512
    visible: true
    color: backgroundColor
    flags: Qt.FramelessWindowHint

    readonly property color backgroundColor: "#ffffff"

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    PathView {
        id: view
        anchors.fill: parent
        anchors.leftMargin: 130
        anchors.rightMargin: 130
        model: ListModel {
            ListElement { source: "qtquickcontrols2-basic.png"; dark: false }
            ListElement { source: "qtquickcontrols2-fusion.png"; dark: false }
            ListElement { source: "qtquickcontrols2-universal-light.png"; dark: false }
            ListElement { source: "qtquickcontrols2-universal-dark.png"; dark: true }
            ListElement { source: "qtquickcontrols2-material-dark.png"; dark: true }
            ListElement { source: "qtquickcontrols2-imagine.png"; dark: false }
            ListElement { source: "qtquickcontrols2-material-light.png"; dark: false }
        }

        highlightRangeMode: PathView.StrictlyEnforceRange
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        pathItemCount: 9

        property real centerX: width / 2
        property real centerY: height * 0.4
        property real delegateSize: 393 / 2

        path: CoverFlowPath {
            pathView: view
        }
        delegate: CoverFlowDelegate {}
    }
}
