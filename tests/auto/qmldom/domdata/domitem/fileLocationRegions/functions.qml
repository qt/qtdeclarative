// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    function a() : int {}

    component A : Item {
        function b(k: int) : int {}
    }

    signal k(int a)
    signal kk(a: int)
}
