pragma Strict
import QtQml

QtObject {
    id: self

    property var thing: self

    function doThing() : bool {
        if (self.thing)
            return true;
        else
            return false;
    }
}
