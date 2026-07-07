// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// A composite type with a non-composite base (Item) and its own VME method and a binding that
// calls it. The reload changes the base type to Rectangle, which shifts the VME method's index.

import QtQuick

Item {
    id: base
    function tag() { return 11 }
    property int reported: base.tag()
}
