// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// "spot" is both a value-type property (QPoint) of ImmediateHost and the id of a sibling
// object. "spot.x" is a plain value-type group property that resolves to the host's "spot"
// property, not to the object with id "spot". The patcher must reset/patch it on the host,
// not on the id object.

import QtQuick
import Qt.Test.PreviewDeferred

ImmediateHost {
    target: Item { id: spot; objectName: "spotItem"; x: 7 }

    spot.x: 100
}
