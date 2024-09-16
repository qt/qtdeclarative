// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.ScrollIndicator {
    id: control
    objectName: "scrollindicator-incomplete"

    contentItem: Item {
        objectName: "scrollindicator-contentItem-incomplete"
    }

    background: Item {
        objectName: "scrollindicator-background-incomplete"
    }
}
