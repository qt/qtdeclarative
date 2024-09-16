// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    function f(x) {
        // sanity check: no break or continue allowed in function body

        for(let i = 0; i < 5; ++i) {
            // break and continue allowed in loop
        }

        switch(x) {
            // no break allowed here
        case 3:
            // break allowed in case
        default:
            // break allowed in default
        case f("helloWorld"):
            // break allowed in moreCase
        }

        helloLabel: {
            // break allowed in labelledstatement
        }

        // combinations:
        combiLabel: {
            // break allowed in labelledstatement
            for(let i = 0; i < 5; ++i) {
                // break and continue allowed in loop

                switch(x) {
                default:
                    // break allowed in default + continue for loop
                }
            }

            switch(x) {
            case 3:
            default:
            case f("helloWorld"):
                // no continue allowed here

                for(let i = 0; i < 5; ++i) {
                    // break and continue allowed in loop
                }
            }
        }

    }

}
