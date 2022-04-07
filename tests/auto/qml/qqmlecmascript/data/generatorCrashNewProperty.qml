// QTBUG-91491
import QtQml 2.15

QtObject {
   property int a: 42
   property int b: 0
   property int c: 0

   function f(myfunc) {
      let gen = myfunc();
      gen["u"] = 0 // Adding members to the generator used to cause crashes when calling next()
      c = gen.next().value
   }

   function refreshA() {
      f(function*() { b = 12; return a });
   }

   Component.onCompleted: refreshA();
}
