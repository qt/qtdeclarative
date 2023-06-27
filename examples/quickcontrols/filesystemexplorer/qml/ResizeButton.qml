// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls
import FileSystemModule

Button {
    required property ApplicationWindow resizeWindow

    icon.width: 20; icon.height: 20
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    rightPadding: 3
    bottomPadding: 3

    icon.source: "../icons/resize.svg"
    icon.color: hovered ? Colors.iconIndicator : Colors.icon

    background: null
    checkable: false
    display: AbstractButton.IconOnly
    onPressed: resizeWindow.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
}
