import Qt.v4 1.0

Result {
    property string s: "foo"
    property bool flag: true

    result: (!flag && s) == false
}
