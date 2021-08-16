import QtQuick

QtObject {
    property string string1: "Hello"

    property int int1: 3

    property double double1: 1

    property real real1: 5.3

    property var var1: null
    property var var2: 3

    property Item item: null

    property Gradient gradient: null

    property bool b1: true
    property bool b2: false
    property bool b3
    b3: true

    property TextInput edit: TextInput { validator: RegularExpressionValidator { regularExpression: /.*/ } }

    property url url1: "http://google.com"

    property color color1: "red"

    property date date1: "2021-08-13T14:16:21.435Z"

    property point point1: "1,2"

    property size size1: "50x50"

    property rect rect1: "10,20,30x30"
}
