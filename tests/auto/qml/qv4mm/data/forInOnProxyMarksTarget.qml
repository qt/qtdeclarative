import QtQml

QtObject {
    property bool wasInUseBeforeRevoke: false
    property bool wasInUseAfterRevoke: false

    Component.onCompleted: {
        let handler = {};
        let target = {prop1: 1, prop2: 2};

        let proxy = Proxy.revocable(target, handler);
        wasInUseBeforeRevoke = __inUse(target)
        target = null;

        for (var prop in proxy.proxy) {
            prop[4] = 10;
            proxy.revoke()
            gc()
            wasInUseAfterRevoke = __inUse()
            break
        }
    }
}
