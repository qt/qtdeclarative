import QtQml
import QtQml.DesignSupport

QtObject {
    id: root

    property Component objectFactory: Component {
        QtObject {
            property int testProp: 11
        }
    }

    property ObjectRegistry singleReg: ObjectRegistry { key: "ImperativeSingle"; target: null }
    property ObjectRegistry multiReg1: ObjectRegistry { key: "ImperativeMulti"; target: null }
    property ObjectRegistry multiReg2: ObjectRegistry { key: "ImperativeMulti"; target: null }

    property QtObject obj1: null
    property QtObject obj2: null

    function registerObjects() {
        obj1 = objectFactory.createObject(null);
        obj2 = objectFactory.createObject(null);

        singleReg.target = obj1;
        multiReg1.target = obj1;
        multiReg2.target = obj2;
    }

    function destroyObjects() {
        obj1.destroy();
        obj2.destroy();
        obj1 = null;
        obj2 = null;
    }
}
