import Qt.v4 1.0

Result {
    property real nan: Number.NaN
    property bool flag: true
    property real subresult: nan && flag

    result: isNaN(subresult)
}
