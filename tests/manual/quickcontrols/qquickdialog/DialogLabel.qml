// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls

Label {
    x: dialog.x + (dialog.width - width) / 2
    y: dialog.y - height
    width: dialog.width
    wrapMode: Label.Wrap

    property Dialog dialog
}
