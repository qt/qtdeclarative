// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadBaseChangeShared/Base.qml but the non-composite base changed from Item to
// Rectangle. Rectangle's larger meta-object shifts the indices of the VME members.

import QtQuick

Rectangle {
    id: base
    property int value: 22
    property alias valueAlias: base.value
}
