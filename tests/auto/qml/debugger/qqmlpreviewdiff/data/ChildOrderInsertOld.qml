// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Two children. The new version inserts a third child between them.
import QtQuick

Item {
    Item { objectName: "alpha"; width: 10 }
    Item { objectName: "gamma"; x: 30 }
}
