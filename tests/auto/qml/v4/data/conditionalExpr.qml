import Qt.v4 1.0

Result {
    property int n: 2
    property int a: n ? 1 : 0
    property int b: if (n) { 1 } else { 0 }
    result: (a && b) ? 0 : 1
}
