import Qt.v4 1.0

Result {
    property bool val1: false
    property bool val2: true
    property bool val3: false

    property bool b1: (true && true && false)
    property bool b2: (true && (false && true))
    property bool b3: ((true && true) && true)
    property bool b4: (val1 && (val2 && val3)) ? true : false

    result: !b1 && !b2 && b3 && !b4
}
