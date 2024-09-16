import QtQml

ListProvider {
    json: [1, "aa", 2, null, undefined]
    strings: json
    stringStrings: json
    property list<string> strings2: json

    Component.onCompleted: {
        console.log("json", json)
        console.log("strings", strings)
        console.log("strings2", strings2)
    }
}
