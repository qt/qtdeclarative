import QtQml

QtObject {
    property int a: (155 >> 2) << (2 << 2);
    property int b: (a + 1) >> 1
    property int c: (b - 2) << 2
    property int d: (a + 3) << (b * 3)
    property int e: (d - 4) >> (c / 2)
}
