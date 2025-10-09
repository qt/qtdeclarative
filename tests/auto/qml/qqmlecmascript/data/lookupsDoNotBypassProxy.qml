import QtQml

QtObject {
    function test_proxy() {
        let base = {
            id: 'baseid',
            name: 'basename',
            length: 42
        };

        let handler = {
            get: function (ao, prop) {
                return Reflect.get(ao, prop);
            }
        };

        let r = new Proxy(base, handler);
        let validCount = 0;
        if (r.id === base.id)
            ++validCount;
        if (r.length === base.length)
            ++validCount;
        if (r.name === base.name)
            ++validCount;
        return validCount;
    }

    property int result: test_proxy()
}
