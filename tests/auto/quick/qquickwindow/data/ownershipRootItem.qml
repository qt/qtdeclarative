import QtQuick 2.0
import QtQuick.Window 2.0 as Window
import Test 1.0

Window.Window {
RootItemAccessor {
  id:accessor
  objectName:"accessor"
  Component.onCompleted:accessor.rootItem();
}

}