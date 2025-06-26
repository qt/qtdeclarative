// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Control {
    id: control
    
    Component.onCompleted: {
        let globalPoints = control.map(control.contentItem.mapToGlobal)
    }
}
