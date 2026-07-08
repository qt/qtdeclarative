// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

Item {
    width: height + 2
    property int answer: 42
    Component.onCompleted: {
        var x = answer;
    }
}
