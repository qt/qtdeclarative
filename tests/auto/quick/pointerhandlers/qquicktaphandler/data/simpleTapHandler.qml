// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.12

Item {
    id: root
    width: 100
    height: 100
    property int tapCount: 0
    TapHandler {
        onTapped: { ++root.tapCount }
    }
}
