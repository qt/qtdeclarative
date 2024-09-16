// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

QtObject {
    function exceptionFail() {
        console.exception("Exception 2");
    }

    Component.onCompleted: {
        try {
            console.exception("Exception 1");
        } catch (e) {
            console.log(e);
        }

        exceptionFail();
    }
}
