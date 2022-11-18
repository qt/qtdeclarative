// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.Pane {
    id: control
    objectName: "pane-incomplete"

    contentItem: Item {
        objectName: "pane-contentItem-incomplete"
    }

    background: Item {
        objectName: "pane-background-incomplete"
    }
}
