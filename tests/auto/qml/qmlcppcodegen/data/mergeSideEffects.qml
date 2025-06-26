import QtQml

QtObject {
    property bool no: false
    property list<int> a: [1]
    property list<int> b: [2]

    property int c: {
        let numbers = a;
        a = [3]; // create side effect affecting "numbers"

        if (no) {
            // Force two branches to be merged on "numbers"
            numbers = b
        }

        // Side effect is still in effect
        return numbers[0];
    }
}
