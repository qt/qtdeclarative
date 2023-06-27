// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
//![0]
import QtQuick

Item {
    Item {
        focus: true
        Keys.onPressed: (event)=> {
            console.log("KeyReader captured:",
                        event.text);
            event.accepted = true;
        }
    }
}
//![0]
