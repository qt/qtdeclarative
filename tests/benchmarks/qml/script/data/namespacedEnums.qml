// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0 as QtQuick

QtQuick.Item {
    function runtest() {
        var a = 0;
        for (var ii = 0; ii < 100000; ++ii)
            a += QtQuick.Text.RichText;
        return a;
    }
}

