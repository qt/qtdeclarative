// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Parent composite type — derives from ExternalGrandparentScript, adds script binding.
import QtQuick

ExternalGrandparentScript {
    property int parentValue: 3
    property int parentTripled: parentValue * 3
}
