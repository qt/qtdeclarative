import QtQuick 2.0

QtObject {
    property string string1: "aaba"
    property string string2: "aa"
    property string string3: "aaab"
    property string string4: "c"

    property bool test1: string1 > string2
    property bool test2: string2 < string1
    property bool test3: string1 > string3
    property bool test4: string3 < string1
    property bool test5: string1 < string4
    property bool test6: string4 > string1

    property bool test7: string1 == "aaba"
    property bool test8: string1 != "baa"
    property bool test9: string1 === "aaba"
    property bool test10: string1 !== "baa"
    property bool test11: string4 == "c"
    property bool test12: string4 != "d"
    property bool test13: string4 === "c"
    property bool test14: string4 !== "d"

    property bool test15: string1 >= string2
    property bool test16: string2 <= string1
    property bool test17: string1 >= string3
    property bool test18: string3 <= string1
    property bool test19: string1 <= string4
    property bool test20: string4 >= string1
    property bool test21: string4 <= "c"
    property bool test22: string4 >= "c"
}

