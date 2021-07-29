import QtQml 2.15

QtObject {
  function test_generator_gc() {
    ((function*() { gc() })()).next();
    ((function*() { gc() })()).next();
    ((function*() { gc() })()).next();
    ((function*() { gc() })()).next();
  }

  Component.onCompleted: () => test_generator_gc()

}
