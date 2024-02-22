// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

AbstractButton {
    id: button
    width: 200
    height: 200
    icon.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
    icon.color: enabled ? "transparent" : "red"

    IconLabel {
        icon: button.icon
        text: button.text
    }
}
