// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
QtObject {
    id: root
    function* gen() { yield 20 }
    function first() { return root.gen().next().value }
}
