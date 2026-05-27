// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Rectangle {
    property string label: "original"
    color: "green"
    Component.onCompleted: console.log("lazy_component_test instantiated label=" + label)
}
