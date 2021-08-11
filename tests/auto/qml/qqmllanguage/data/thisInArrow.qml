import QtQml

QtObject {
    id: root

    property int width: 43
    Component.onCompleted: () => { console.log(this.width); }

    property var arrow: () => { return this; }
    property var func: function() { return this; }

    property QtObject child: QtObject {
        property var aa;
        property var ff;

        Component.onCompleted: {
            root.arrowResult = root.arrow();
            root.funcResult = root.func();

            var a = root.arrow;
            root.aResult = a();
            var f = root.func;
            root.fResult = f();

            aa = a;
            root.aaResult = aa();

            ff = f;
            root.ffResult = ff();
        }
    }

    property var arrowResult
    property var funcResult
    property var aResult
    property var fResult
    property var aaResult
    property var ffResult
}
