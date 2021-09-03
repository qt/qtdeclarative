import QtQml
import QtQuick 2

QtObject {
    // THIS NEEDS TO BE LAST
    largeBinding: {
        var x = 300;
        console.log(x);
    }

    small1: 3
    small2: foo
    smallButNeedsBraces: if (foo) {
        bar();
    }
}
