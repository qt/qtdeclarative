import QtQml

QtObject {
   @Deprecated {
    reason: "Test"
   }
   property int deprecated: 10

   Component.onCompleted: {
       console.log(deprecated);
   }
}
