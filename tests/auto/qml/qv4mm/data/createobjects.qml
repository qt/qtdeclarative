// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
