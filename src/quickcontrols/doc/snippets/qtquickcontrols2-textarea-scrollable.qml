// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

Item {
    width: 100
    height: 100

    Binding { target: view.ScrollBar.vertical; property: "active"; value: true }

    //! [1]
    ScrollView {
        id: view
        anchors.fill: parent

        TextArea {
            text: "TextArea\n...\n...\n...\n...\n...\n...\n"
        }
    }
    //! [1]
}
