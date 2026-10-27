// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite type with a non-composite base (Item), a property and an alias to it. The reload
// changes the base type to Rectangle, which shifts the indices of the VME members.

import QtQuick

Item {
    id: base
    property int value: 11
    property alias valueAlias: base.value
}
