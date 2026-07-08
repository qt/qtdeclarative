// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// An extra-file component that is a Control, i.e. its contentItem is a deferred
// property. Editing this file repeatedly must keep recreating the deferred content.

import QtQuick
import QtQuick.Controls

AbstractButton {
    id: control
    property string label: "initial"

    contentItem: Text {
        objectName: "childContent"
        text: control.label
    }
}
