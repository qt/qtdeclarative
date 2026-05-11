// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// QTBUG-145794: Delegates created by Repeater live in child contexts.
// The debug service must expose them in the LIST_OBJECTS_R response.
Item {
    Repeater {
        model: 3
        delegate: Item { objectName: "delegateItem" + index }
    }
}
