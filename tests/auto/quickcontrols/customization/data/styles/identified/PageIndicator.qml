// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.PageIndicator {
    id: control
    objectName: "pageindicator-identified"

    contentItem: Item {
        id: contentItem
        objectName: "pageindicator-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "pageindicator-background-identified"
    }
}
