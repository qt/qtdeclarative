// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Same as HotReloadBaseChangeDerived/Base.qml but the non-composite base changed from Item to
// Rectangle. Rectangle's larger meta-object shifts tag()'s VME method index.

import QtQuick

Rectangle {
    id: base
    function tag() { return 22 }
    property int reported: base.tag()
}
