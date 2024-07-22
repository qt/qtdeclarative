// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
Item {
    property list<QtObject> objects: [QtObject{}, QtObject{}]

    function a() {
        const t = Date.now()
        const a = new Date();
        const aa = Array.prototype.push(1)
        const KK = Math.random();
    }
}
