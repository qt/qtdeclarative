import QtQml

QtObject {
    property var testObj
    function use() { return testObj.has(1) ? 25 : 26; }
}
