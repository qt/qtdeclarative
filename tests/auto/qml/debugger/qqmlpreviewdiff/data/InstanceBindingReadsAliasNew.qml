// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as Old but bgColor changed.

import QtQuick

CompositeBaseWithAliases {
    width: 400
    height: 400
    header.text: "Original"

    property string headerInfo: header.text + " (" + header.width + ")"
    property color bgColor: "red"
}
