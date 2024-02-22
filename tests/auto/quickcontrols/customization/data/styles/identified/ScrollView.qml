// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T

T.ScrollView {
    id: control
    objectName: "scrollview-identified"

    background: Item {
        id: background
        objectName: "scrollview-background-identified"
    }
}
