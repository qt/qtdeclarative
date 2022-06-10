import QtQml
import QtQuick
import QQmlJSScopeTests

Text {
    id: root

    // js func
    function jsFunc() { return true; }

    // js func with typed params
    function jsFuncTyped(url: string) { return url + "/"; }

    // script binding (one-line and multi-line)
    elide: Text.ElideLeft
    color: {
        if (root.truncated) return "red";
        else return "blue";
    }

    property int newProperty: {
        var callable = () => { return 42; };
        return callable();
    }

    // group prop script binding (value type and non value type)
    font.pixelSize: (40 + 2) / 2
    anchors.bottomMargin: 11 + 1
    anchors.topMargin: {
        var divideBy4 = function (x) { return x / 4; };
        return divideBy4(44);
    }

    // attached prop script binding
    Keys.enabled: root.truncated ? true : false

    // property change handler ({} and function() {} syntaxes)
    property int p0: 42
    property Item p1
    property string p2
    onP0Changed: console.log("p0 changed");
    onP1Changed: {
        console.log("p1 changed");
    }
    onP2Changed: function() {
        console.log("p2 changed:");
        console.log(p2);
    }

    // signal handler ({} and function() {} syntaxes)
    signal mySignal0(int x)
    signal mySignal1(string x)
    signal mySignal2(bool x)
    onMySignal0: console.log("single line", x);
    onMySignal1: {
        var identity = (x) => { return x; };
        console.log("mySignal1 emitted:", identity(x));
    }
    onMySignal2: function (x) {
        var returnString = function() { return "mySignal2 emitted:"; };
        console.log(returnString(), x);
    }

    // var property assigned a js function
    property var funcHolder: function(x, y) { return x + y; }

    component InlineType : Item {
        function jsFuncInsideInline() { return 42; }
        objectName: "inline" + " " + "component"
        Item { // inside inline component
            y: 40 / 2
        }
    }
    InlineType {
        function jsFuncInsideInlineObject(x: real) { console.log(x); }
        Item { // outside inline component
            focus: root.jsFunc();
        }
    }

    TableView {
        delegate: Text {
            signal delegateSignal()
            onDelegateSignal: { root.jsFunc(); }
        }

        component InternalInlineType : Rectangle {
            function jsFuncInsideInline2() { return 43; }
        }

        property var funcHolder2
        funcHolder2: function(x) { return x * 2; }
    }

    ComponentType {
        property string foo: "something"
        onFooChanged: console.log("foo changed!");
    }

    GridView {
        delegate: ComponentType {
            function jsFuncInsideDelegate(flag: bool) { return flag ? "true" : "false"; }
        }
    }

    property list<Item> itemList: [
        Text {
            function jsInsideArrayScope() { return 42; }
        },
        Item {
            property var x123: function(a) { return a + 77; }
            function jsInsideArrayScope2(flag) {
                var closure = () => { return "foobar"; };
                if (flag)
                    return closure;
                return () => { return "bazbar"; };
            }
        },
        Rectangle {} // dummy
    ]

    // translations:
    property string translated1: qsTr("bad but ok")
    property string translated2: qsTrId("bad_but_ok_id")
    property string translated3: qsTr("bad but ok") + "?"
    property string translated4: qsTrId("bad_but_ok_id") + "?"

    property string translated5
    property string translated6
    translated5: qsTr("bad but ok")
    translated6: qsTrId("bad_but_ok_id")

    // function with nested one
    function jsFunctionReturningFunctions() {
        return [ function() { return 42 }, () => { return "42" } ];
    }

    // function with Qt.binding()
    function bindText() {
        root.text = Qt.binding( function() { return jsFuncTyped("foo") + "bar"; } );
    }

    // special code which fails somehow:
    TypeWithProperties {
        onAChanged: console.log("x");
    }

    TypeWithProperties {
        onAChanged: function() { }
    }
}
