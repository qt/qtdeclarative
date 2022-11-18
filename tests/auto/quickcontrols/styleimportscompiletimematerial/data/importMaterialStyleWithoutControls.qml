// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls.Material

ApplicationWindow {
    title: "Test Application Window"
    width: 400
    height: 400

    property alias button: button

    Button {
        id: button
        text: "Material Button"
    }
}
