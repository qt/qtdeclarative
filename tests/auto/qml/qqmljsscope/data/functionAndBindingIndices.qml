import QtQml
import QtQuick

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

    // group prop script binding (value type and non value type)
    font.pixelSize: (40 + 2) / 2
    anchors.topMargin: 44 / 4

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
        console.log("mySignal1 emitted:", x);
    }
    onMySignal2: function (x) {
        console.log("mySignal2 emitted:", x);
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

        property var prop: function(x) { return x * 2; }
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
}
