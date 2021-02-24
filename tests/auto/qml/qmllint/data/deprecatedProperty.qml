import QtQml

QtObject {
   @Deprecated {}
   property int deprecated: 10

   Component.onCompleted: {
       console.log(deprecated);
   }
}
