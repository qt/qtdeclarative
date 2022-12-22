// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultTreeViewDelegate {
    id: control

    palette.highlight: "#2f64e1"
    palette.highlightedText: "white"

    background: Rectangle {
        color: control.highlighted ? control.palette.highlight
               : (control.treeView.alternatingRows && control.row % 2 !== 0
               ? control.palette.alternateBase : control.palette.base)
        // Ideally we want a rounded background for the whole row, also when
        // there are more than one column. But until Rectangle gains support
        // for corners with individual radii, we simplify it (QTBUG-48774)
        radius: control.row === control.treeView.currentRow && control.treeView.columns === 1 ? 5 : 0

        readonly property bool __ignoreNotCustomizable: true
    }
}

