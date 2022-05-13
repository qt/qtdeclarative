// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QmlTime 1.0 as QmlTime

Item {

    QmlTime.Timer {
        component: Component {
            ParallelAnimation {
                NumberAnimation { }
                NumberAnimation { }
                NumberAnimation { }
                ColorAnimation { }
                SequentialAnimation {
                    PauseAnimation { }
                    ScriptAction { }
                    PauseAnimation { }
                    ScriptAction { }
                    PauseAnimation { }
                    ParallelAnimation {
                        NumberAnimation { }
                        SequentialAnimation {
                            PauseAnimation { }
                            ParallelAnimation {
                                NumberAnimation { }
                                NumberAnimation { }
                            }
                            NumberAnimation { }
                            PauseAnimation { }
                            NumberAnimation { }
                        }
                        SequentialAnimation {
                            PauseAnimation { }
                            NumberAnimation { }
                        }
                    }
                }
            }
        }
    }

}
