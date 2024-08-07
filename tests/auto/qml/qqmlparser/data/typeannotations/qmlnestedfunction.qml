import QtQuick 2.12 as MyQuick
MyQuick.Item {
    function factory(param: string) : MyQuick.Item {
        function nested(foo: string) {
            return this
        }
        return nested()
    }
}
