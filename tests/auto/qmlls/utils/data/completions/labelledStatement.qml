import QtQuick

Item {
    function f(x) {
        label1: f(f(x))

        nestedLabel1: for (let i = 0; i < 3; ++i) {
            nestedLabel2: for (let j = 0; j < 3; ++j) {
                if (i === 1 && j === 1) {
                    continue nestedLabel1;
                }
            }
        }

        multilabel1:
        multilabel2: f(1 + f(x))

        return x + y
    }
}
