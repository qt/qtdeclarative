import QtQuick 1.0

QtObject {
    property int test: if (b == 1) 123; else 321;
    property int b: 1
}
