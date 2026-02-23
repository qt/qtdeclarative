// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Instance-level binding that reads from a base type alias.
// Tests that alias resolution works in instance-level bindings after rebuild.

import QtQuick

CompositeBaseWithAliases {
    width: 400
    height: 400
    header.text: "Original"

    // This binding reads from the 'header' alias (resolved in base type context)
    // and uses it in the instance level.
    property string headerInfo: header.text + " (" + header.width + ")"
    property color bgColor: "blue"
}
