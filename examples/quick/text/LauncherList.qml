// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ListView {
    id: root

    signal exampleClicked(url source)

    delegate: SimpleLauncherDelegate {
        onClicked: root.exampleClicked(Qt.resolvedUrl(modelData.source))
    }
    ScrollBar.vertical: ScrollBar {}
}
