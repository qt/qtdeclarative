// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls.Imagine

ToolBar {
    // Seems to be necessary to get the default assets to be used here,
    // though it should inherit the window's path
    Imagine.path: defaultImaginePath
}
