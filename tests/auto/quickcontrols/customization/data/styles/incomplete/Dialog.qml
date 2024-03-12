// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.Dialog {
    id: control
    objectName: "dialog-incomplete"

    contentItem: Item {
        objectName: "dialog-contentItem-incomplete"
    }

    background: Item {
        objectName: "dialog-background-incomplete"
    }
}
