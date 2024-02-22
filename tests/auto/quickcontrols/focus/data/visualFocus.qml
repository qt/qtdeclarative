// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

Column {
    width: 400
    height: 400
    Button {
        text: "Button"
        property bool showFocus: visualFocus
    }
    TextField {
        text: "TextField"
    }
}
