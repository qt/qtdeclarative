import QtQml 2

QtObject {
  id: root
  property int sum
  Component.onCompleted: {
    const target = { prop1: 1, prop2: 2, prop3: 3 };
    const handler = {
      get: function(target, key) {
        return target[key]+1;
      },
      ownKeys: function() {
        return ["prop1", "prop3"];
      },
      getOwnPropertyDescriptor: function(target, key) {
        return {
          value: this.get(target, key),
          enumerable: true,
          configurable: true
        };
      }
    };
    const proxy = new Proxy(target, handler);
    for (var prop in proxy) {
      root.sum += proxy[prop] // prop2 gets skipped, the values of 1 and 3 get incremented
    }
    // so root.sum should be 6 now
  }
}
