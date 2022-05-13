// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
