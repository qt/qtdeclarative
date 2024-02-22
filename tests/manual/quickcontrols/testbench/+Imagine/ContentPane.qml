// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick.Controls.Imagine

Pane {
    Imagine.path: settings.useCustomImaginePath && settings.imaginePath.length > 0 ? settings.imaginePath : undefined
}
