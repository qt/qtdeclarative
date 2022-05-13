// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.MenuSeparator {
    id: control
    objectName: "menuseparator-identified"

    contentItem: Item {
        id: contentItem
        objectName: "menuseparator-contentItem-identified"
    }

    background: Item {
        id: background
        objectName: "menuseparator-background-identified"
    }
}
