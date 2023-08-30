import QtQml

QtObject {

    function evil({ hello = "world", x = 42 },
                  [n = 42, m = 43, o = 44],
                  { destructuring, is = {a, lot, of}, fun = 42 } = {destructuring : 123, is : {x : 123}, fun : 456}) {
    }
}
