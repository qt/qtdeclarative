// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Templates as T

T.CheckBox {
    id: control
    objectName: "checkbox-incomplete"

    indicator: Item {
        objectName: "checkbox-indicator-incomplete"
    }

    contentItem: Item {
        objectName: "checkbox-contentItem-incomplete"
    }
}
