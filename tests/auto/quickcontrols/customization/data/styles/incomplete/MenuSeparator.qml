// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.MenuSeparator {
    id: control
    objectName: "menuseparator-incomplete"

    contentItem: Item {
        objectName: "menuseparator-contentItem-incomplete"
    }

    background: Item {
        objectName: "menuseparator-background-incomplete"
    }
}
