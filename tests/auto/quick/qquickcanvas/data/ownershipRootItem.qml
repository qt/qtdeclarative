import QtQuick 2.0
import QtQuick.Window 2.0 as Window

Window.Window {
RootItemAccessor {
  id:accessor
  objectName:"accessor"
  Component.onCompleted:accessor.rootItem();
}

}