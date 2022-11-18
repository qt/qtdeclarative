// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls

Dialog {
    id: root
    x: previousDialog ? previousDialog.x + previousDialog.width + space : 0
    y: previousDialog ? previousDialog.y : 0
    closePolicy: Dialog.NoAutoClose
    visible: true

    property Dialog previousDialog
    property int space: dialogSpacing

    Marker {
        parent: root.footer.contentItem
        visible: visualizeDialogButtonBoxContentItem
        text: "footer.contentItem"
    }
    Marker {
        parent: root.footer
        visible: visualizeDialogButtonBox
        text: "footer"
        border.color: "red"
    }
}
