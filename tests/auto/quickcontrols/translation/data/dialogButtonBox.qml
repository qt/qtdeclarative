// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Item {
    property Dialog dialog: Dialog {
        width: 300
        height: 300
        visible: true
        standardButtons: DialogButtonBox.Save | DialogButtonBox.Discard
    }
}
