// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

// moving lines in here requires fixing tst_qqmlconsole.cpp
QtObject {
    id: root

    function tracing()
    {
        console.trace();
    }

    Component.onCompleted: {
        tracing();
    }
}
