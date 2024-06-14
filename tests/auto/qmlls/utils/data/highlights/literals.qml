// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int a: 123
    property string b: "single"
    property string c: "multi
    line string";
    property bool d: true
    property var e: null
    property string d: "multi\n\n
    newlines\n\n
    "
}
