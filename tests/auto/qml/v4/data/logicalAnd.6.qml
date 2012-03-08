import Qt.v4 1.0

Result {
    property string s: ""
    property bool flag: true
    property string subresult: s && flag

    result: subresult === ""
}
