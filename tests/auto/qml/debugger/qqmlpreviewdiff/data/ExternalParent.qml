// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Parent composite type — derives from ExternalGrandparent, adds its own properties.
import QtQuick

ExternalGrandparent {
    property int parentValue: 50
    property string parentLabel: "parent"
}
