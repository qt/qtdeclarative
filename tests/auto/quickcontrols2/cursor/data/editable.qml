// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

MouseArea {
    width: column.width
    height: column.height
    cursorShape: Qt.ForbiddenCursor

    Column {
        id: column
        padding: 10
        spacing: 10

        ComboBox {
            editable: true
            model: "ComboBox"
        }
        SpinBox {
            editable: true
        }
    }
}
