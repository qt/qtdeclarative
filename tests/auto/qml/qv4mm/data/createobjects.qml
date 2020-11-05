/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQml 2.2

QtObject {
    property var objects: []
    property int numChecked: 0
    property var c: null
    property bool ok: false

    function container() {
        var objs

        return {
            "check": function() {
                for (var i = 0; i < 1000; ++i) {
                    if (objs[i][0] !== 1 || objs[i][1] !== 2 || objs[i][2] !== 3)
                        return false;
                }
                return true;
            },

            "generate": function() {
                objs = [];
                for (var i = 0; i < 1000; ++i)
                    objs[i] = [1, 2, 3]
            }
        }
    }

    property Component itemComponent: Component {
        QtObject {}
    }

    property Component triggerComponent: Component {
        QtObject {
            Component.onDestruction: {
                for (var i = 0; i < 1000; ++i)
                    objects[i] = itemComponent.createObject();
                c = container();
                c.generate();
            }
        }
    }

    Component.onCompleted: {
        triggerComponent.createObject();
        gc();
        for (var i = 0; i < 1000; ++i) {
            if (objects[i] !== undefined)
                ++numChecked;
        }
        ok = c.check();
    }
}
