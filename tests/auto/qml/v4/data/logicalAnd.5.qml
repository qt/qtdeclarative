import Qt.v4 1.0

Result {
    property bool flag: true

    result: (null && flag) == null
}
