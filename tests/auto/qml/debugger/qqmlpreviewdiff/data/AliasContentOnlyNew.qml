// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    width: 400
    height: 400
    property alias nestedAlias: another

    Item {
        id: nested
        width: 100
    }

    Item {
        id: another
    }
}
