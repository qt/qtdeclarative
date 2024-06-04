import QtQuick

Item {
    function its(string, ...args)  { return "magic!" }

    function f(b, d) {
        let normality = `a${b}c${d}e`;
        let noHead = `${b}c${d}e`;
        let noTail = `a${b}c${d}`;
        let noHeadNoTail = `${b}c${d}`;
        let empty = ``;
        let manyLines = `line 1
line 2
line 3`;
        let afterManyLines = ""
        let taggedTemplate = its`a normal string`
    }
}
