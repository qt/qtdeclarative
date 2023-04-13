pragma ValueTypeBehavior: Copy, Addressable
import QtQml

QtObject {
    id: root

    property list<double> numbers: {
        var result = [];
        for (var i = 0; i < 10; ++i)
            result[i] = i;
        return result;
    }

    property rect r: ({x: 1, y: 2, width: 3, height: 4})

    function evil() : double {
        var numbers = root.numbers;
        root.numbers = [];
        var a = 0;
        for (var j = 0; j < 10; ++j) {
            a += numbers[j];
        }
        return a;
    }

    function fvil() : double {
        var r = root.r;
        root.r = {x: 5, y: 6, width: 7, height: 8};
        return r.x;
    }

    property double e: evil()
    property double f: fvil()
}
