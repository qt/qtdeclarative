// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite type that extends CompositeBaseWithAliases, adding another
// alias layer. Tests deep composite alias resolution during rebuild.

import QtQuick

CompositeBaseWithAliases {
    id: derived

    property alias headerText: derived.header.text
    property alias contentWidth: derived.content.width

    property int counter: 0
}
