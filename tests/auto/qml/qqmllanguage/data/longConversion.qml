import QtQml
import Test

QtObject {
    property bool testProp: GetterObject.getFalse()
    property bool testQProp: GetterObject.getQFalse()

    property bool fromLocal
    property bool fromQLocal

    property bool fromBoolean
    property bool fromQBoolean

    Component.onCompleted:  {
        let l = GetterObject.getFalse();
        fromLocal = l;

        let b = Boolean(l);
        fromBoolean = b;

        let ql = GetterObject.getQFalse();
        fromQLocal = ql;


        let qb = Boolean(ql);
        fromQBoolean = qb;
    }
}
