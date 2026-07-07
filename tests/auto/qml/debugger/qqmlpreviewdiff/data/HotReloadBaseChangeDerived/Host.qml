// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Instantiates a derived type (component DerivedItem: Base {}) several times. Base appears only as
// a composite base in the instances' VME chain, so their property caches derive Base's cache.

import QtQuick

Item {
    component DerivedItem: Base {}

    DerivedItem { objectName: "d1" }
    DerivedItem { objectName: "d2" }
    DerivedItem { objectName: "d3" }
}
