// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

ListView {
    id: listView
    anchors.fill: parent
    model: 20
    delegate: ItemDelegate {
        width: listView.width
        text: modelData
    }

    ScrollBar.vertical: ScrollBar {
        //! [1]
        policy: listView.contentHeight > listView.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
        //! [1]
    }
}
