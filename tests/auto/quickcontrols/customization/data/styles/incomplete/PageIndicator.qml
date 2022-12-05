// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.PageIndicator {
    id: control
    objectName: "pageindicator-incomplete"

    contentItem: Item {
        objectName: "pageindicator-contentItem-incomplete"
    }

    background: Item {
        objectName: "pageindicator-background-incomplete"
    }
}
