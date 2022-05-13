// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QmlTime 1.0 as QmlTime

Item {
    QmlTime.Timer {
        component: Component {
            Item {
                Rectangle { }
                Rectangle { }
                Item {
                    Image { }
                    Text { }
                }

                Item {
                    Item {
                        Image { }
                        Image { }
                        Item {
                            Image { }
                            Image { }
                        }
                    }

                    Item {
                        Item {
                            Text { }
                            Text { }
                        }
                        Text { }
                    }
                }
                MouseArea { }
            }
        }
    }
}
