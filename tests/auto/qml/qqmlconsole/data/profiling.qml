// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

QtObject {
    Component.onCompleted: {
        console.profile("profile1");
        console.time("timer1");
        console.timeEnd("timer1");
        console.profileEnd("profile1");
    }
}
