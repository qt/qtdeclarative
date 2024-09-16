// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: rootId
    Rectangle {
        function f() {
            if (widthChanged())
                rootId.widthChanged();
            return rootId.x
        }
    }

    function t() {
        rootId.widthChanged();
    }
}
