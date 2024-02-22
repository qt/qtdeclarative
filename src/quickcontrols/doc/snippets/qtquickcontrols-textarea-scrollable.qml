// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
