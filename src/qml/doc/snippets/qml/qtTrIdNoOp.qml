// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Item {
    property string greetingId: QT_TRID_NOOP("hello_id")

    Text { text: qsTrId(greetingId) }
}
//![0]
