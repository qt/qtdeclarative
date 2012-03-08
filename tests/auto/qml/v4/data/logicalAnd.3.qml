import Qt.v4 1.0

Result {
    property string s: ""
    property bool flag: true

    result: (s && flag) == ""
}
