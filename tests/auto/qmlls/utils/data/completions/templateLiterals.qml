// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int helloProperty: 42
    function f() {
        let x = `hello${helloProperty}World!`;
        let y = f`hello${helloProperty}World!`;
    }
}
