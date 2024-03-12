// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import StyleThatImportsMaterial

ApplicationWindow {
    title: "Test Application Window"
    width: 400
    height: 400

    property alias button: button

    Button {
        id: button
        text: "QmlOnly Button"
    }
}
