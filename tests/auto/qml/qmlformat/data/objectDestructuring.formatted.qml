import QtQml

QtObject {

    Component.onCompleted: {
        set1();
    }

    function set1() {
        const array = [1, 2, 3, 4];
        const [a, b] = [1, 2];
        const [aa, , bb] = array;
        const [aaa = 23, bbb] = array;
        const [a1, b1, ...rest1] = array;
        const [a2, , b2, ...rest2] = array;
        const [a3, b3, ...{
                pop,
                push
            }] = array;
        const [a4, b4, ...[c, d]] = array;
        const obj = {
            _a: 1,
            _b: 2
        };
        const {
            a5,
            b5
        } = obj;
        const {
            a6: a_,
            b6: b1_
        } = obj;
        const {
            a7: a11 = 4,
            b11 = 34,
            c1: b111,
            d1
        } = obj;
        let key = a;
        const {
            [key]: a___
        } = obj;
    }

    function set2() {
        // declare first
        let a, b, a1, b1, c, d, rest, pop, push;
        const array = [1, 2, 3, 4];
        [a, b] = array;
        [a, , b] = array;
        [a = aDefault, b] = array;
        [a, b, ...rest] = array;
        [a, , b, ...rest] = array;
        [a, b, ...{
                pop,
                push
            }] = array;
        [a, b, ...[c, d]] = array;
        const obj = {
            _a: 1,
            _b: 2
        };
        ({
                a,
                b
            } = obj); // brackets are required
        ({
                a: a1,
                b: b1
            } = obj);
    }
}
