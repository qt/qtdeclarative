// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QtQuick 2.0
import QmlTime 1.0 as QmlTime

Item {

    QmlTime.Timer {
        component: Component {
            Item {
                resources: [
                    Rectangle { },
                    Rectangle { },
                    Item { },
                    Image { },
                    Text { },
                    Item { },
                    Item { },
                    Image { },
                    Image { },
                    Row { },
                    Image { },
                    Image { },
                    Column { },
                    Row { },
                    Text { },
                    Text { },
                    Text { },
                    MouseArea { }
                ]

            }
        }
    }

}
