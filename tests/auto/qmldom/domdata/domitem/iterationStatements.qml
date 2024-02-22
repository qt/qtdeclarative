// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {

    function whileStatement() {
        const i = 10;
        while (i > 0) {
            i = i -1;
            while ( i > 100) {}
        }

        while (i > 0) i = i-1
    }

    function doWhile() {
        let a = 7;
        do {
            const b = a;
            a = a - 1;
        } while (a > 0)

        do a = a-1; while (a > 0)
    }

    function forOf() {
        const iterable = [[1,2], [3,4],]
        for (var [a,b] of iterable) {

            let t;
            for (const [a1, , a2, ...rest] of array) {

            }
            for (const k of [1,2,3,4,,,]) {
                t += k;
            }
            for (t of a) {
                {}
            }
            for (t of a) t += k
        }
    }

    function forIn() {
        const enumerable = {
            list: [1, 2, 3, 4, 5],
            name: 'John',
            age: 25
        };

        for (var [a,b,c,d] in enumerable) {
            let t;
            for (t in enumerable) {
                {}
            }
            for (const [a1, , a2, ...rest] in enumerable.list) {

            }
            for (let t in enumerable) t += k
        }
    }
}
