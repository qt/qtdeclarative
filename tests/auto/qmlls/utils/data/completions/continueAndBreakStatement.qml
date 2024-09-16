// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    function f(x) {
        label1: f(f(x))

        nestedLabel1: for (let i = 0; i < 3; ++i) {
            nestedLabel2: for (let j = 0; j < 3; ++j) {
                continue nestedLabel1;
                break nestedLabel2;
            }
        }

        multiLabel1:
        multiLabel2: {
            f(1 + f(x))
            continue multiLabel1
            break multiLabel2
        }

        for(;;) {
            continue ;
            break ;
        }

        return x + y
    }
}
