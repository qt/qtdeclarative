import QtQuick 2.0

QtObject {
    property QtObject myprop1: null
    property QtObject myprop2: QtObject {}
    property real zero: 0
    property bool falseProp: false

    property bool test1: myprop1 == false
    property bool test2: myprop1 == null
    property bool test3: 5 == myprop1
    property bool test4: null == myprop1
    property bool test5: myprop1 != false
    property bool test6: myprop1 != null
    property bool test7: 5 != myprop1
    property bool test8: null != myprop1

    property bool test9: myprop2 == false
    property bool test10: myprop2 == null
    property bool test11: 5 == myprop2
    property bool test12: null == myprop2
    property bool test13: myprop2 != false
    property bool test14: myprop2 != null
    property bool test15: 5 != myprop2
    property bool test16: null != myprop2

    property bool test17: myprop1 == myprop1
    property bool test18: myprop1 != myprop1
    property bool test19: myprop1 == myprop2
    property bool test20: myprop1 != myprop2
    property bool test21: myprop2 == myprop2
    property bool test22: myprop2 != myprop2

    property bool test23: myprop1 == "hello"
    property bool test24: myprop1 != "hello"
    property bool test25: myprop2 == "hello"
    property bool test26: myprop2 != "hello"

    property bool test27: falseProp == zero
    property bool test28: falseProp != zero
    property bool test29: falseProp == 1
    property bool test30: falseProp != 1
    property bool test31: true == zero
    property bool test32: true != zero
    property bool test33: true == 1
    property bool test34: true != 1

    property bool test35: "a\
b" === "ab"
}

