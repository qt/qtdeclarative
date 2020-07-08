import QtQml
import QtQuick 2

QtObject {
    small1: 3
    small2: foo
    smallButNeedsBraces: {
        if (foo) {
            bar();
        }
    }
    // THIS NEEDS TO BE LAST
    largeBinding: {
        var x = 300;
        console.log(x);
    }
}
