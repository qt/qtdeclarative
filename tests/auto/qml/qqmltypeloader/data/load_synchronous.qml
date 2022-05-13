// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml 2.2

QtObject {
    id: top

    Component.onCompleted: {
        Qt.createQmlObject('QtObject {}', top, 'nonprotocol:');
    }
}
