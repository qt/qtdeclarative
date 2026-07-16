// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Analog of CustomButton.qml: derives from the form and adds a binding that
// depends on the form's "col" property (like CustomButton's gradient binding).
import QtQuick

RevertColorFormBase {
    property string effective: (col === "green") ? "green-effect" : "grey-effect"
}
