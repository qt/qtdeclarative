// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.14

Flickable {
    id: flickable
    // Deliberately has no size set.

    Text {
        text: flickable.visibleArea.xPosition
    }
}
