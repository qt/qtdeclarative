// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

pragma Singleton

import QtCore

Settings {
    // The properties here are given default values to account for the first run of the application.
    // After the application has run once, future values will come from the stored settings.
    property bool showDoneTasks: true
    property int maxTasks: 30
    property int fontSize: 18
}
