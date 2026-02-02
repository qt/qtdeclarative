// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
Item {
    property string name: "world"
    function test() {
        function tag(strings) { return strings.join("") }
        return tag`hello ${name} today`
    }
}
