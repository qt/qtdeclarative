import QtQuick 2.0
import QtQuick as QQ

Zzz {
    id: root
    width: height
    Rectangle {
        color: "green"
        anchors.fill: parent
        width: root.height
        height: root.foo.height

    }

    function lala() {}
    property Rectangle foo: Rectangle{ height: 200 }
    function longfunction(a, b, c = "c", d = "d"): string {
        return "hehe: " + c + d
    }

    // documentedFunction: is documented
    // returns 'Good'
    function documentedFunction(arg1, arg2 = "Qt"): string {
        return "Good"
    }
    QQ.Rectangle {
        color:"red"
    }

    Item {
        id: someItem
        property int helloProperty
    }

    function parameterCompletion(helloWorld, helloMe: int) {
        let helloVar = 42;
        let result = someItem.helloProperty + helloWorld;
        return result;
    }

    component Base: QtObject {
        property int propertyInBase
        function functionInBase(jsParameterInBase) {
            let jsIdentifierInBase;
            return jsIdentifierInBase;
        }
    }

    Base {
        property int propertyInDerived
        function functionInDerived(jsParameterInDerived) {
            let jsIdentifierInDerived;
            return jsIdentifierInDerived;
        }

        property Base child: Base {
            property int propertyInChild
            function functionInChild(jsParameterInChild) {
                let jsIdentifierInChild;
                return someItem.helloProperty;
            }
        }
    }
    function test1() {  for (myvar = 42; i<0; ++i){}  console.log(myvar);}
    function test2() {  for (var myvar = 42; i<0; ++i){}  console.log(myvar);}
}
