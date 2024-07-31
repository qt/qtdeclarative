// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    function k() {
    }

    function mafik() {
        var patron = 34;
        const upperLimit = 42;
        do {
            ++patron;
            if (patron < 2)
                continue;
            else
                ++patron;
        } while (patron < upperLimit)
        switch (patron) {
        case 1:
            return 23;
        default:
            break;
        }
        try {
            {}
        } catch (error) {
            {}
        } finally {}
        for (const a in [1, 2, 3]) {
            throw 2;
        }
    }

    enum Test {
        LOG
    }

    readonly property int t: 34
    signal tt
    required property int k

    signal kkk(string a)
    signal yyy(a: string)

    function ttt() {

    }

function createComplexExpression(...objects) {
    // Create an object that holds some data
    let data = {
        a: 5,
        b: 10,
        c: 3
    };

    // Create a complex expression using the data object
    let expression = ((data.a + data.b * data.c) / (data.a - data.b)) ** data.c;

    return expression;
}

    function set1() {
        const array = [1,2,3,4];
        const [a, b] = [1,2];
        const [aa, , bb] = array;
        const [aaa = 23, bbb] = array;
        const [a1, b1, ...rest1] = array;
        const [a2, , b2, ...rest2] = array;
        const [a3, b3, ...{ pop, push }] = array;
        const [a4, b4, ...[c, d]] = array;

        const obj = {_a:1,_b:2};
        const { a5, b5 } = obj;
        const { a6: a_, b6: b1_ } = obj;
        const { a7: a11 = 4, b11 = 34, c1: b111, d1 } = obj;
        let key = a;
        const { [key]: a___ } = obj;
    }

    function set2(s : int) : int {
        // declare first
        let a, b, a1, b1, c, d, rest, pop, push;
        const array = [1,2,3,4];
        [a, b] = array;
        [a, , b] = array;
        [a = aDefault, b] = array;
        [a, b, ...rest] = array;
        [a, , b, ...rest] = array;
        [a, b, ...{ pop, push }] = array;
        [a, b, ...[c, d]] = array;

        const obj = {_a:1,_b:2};
        ({ a, b } = obj); // brackets are required
        ({ a: a1, b: b1 } = obj);

        const complicatedObject = {
            a: 1,
        b: {
            c: 2,
            d: {
                e: 3,
                f: [4, 5, 6]
            }
            },
            g: [7, 8, 9]
        };

        const { patron, b: { mafik, d: { e, f: [ , secondF, ...restF ] } }, g: [ firstG, ...restG ] } = complicatedObject;
    }

    function someKeywords() {
        const a = this.x;
        super()
        super.f()
        return new Date ();
    }
}

