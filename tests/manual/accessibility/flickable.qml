// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0


    ListView {
        id : content
        width: 300
        height: 200
//        KeyNavigation.up: gridMenu; KeyNavigation.left: contextMenu; KeyNavigation.right: list2

        model : 200
        delegate : Text { text : "foo" + index; height : 50 }
    }
