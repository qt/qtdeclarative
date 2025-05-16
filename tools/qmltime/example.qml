// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick 2.0
import QmlTime 1.0 as QmlTime

Item {

    property string name: "Bob Smith"

    QmlTime.Timer {
        component: Item {
            Text { text: name }
        }
    }
}

