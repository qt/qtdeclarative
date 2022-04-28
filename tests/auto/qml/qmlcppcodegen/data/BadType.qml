import QtQuick

Item {
    required property OkType picker
    property bool useListDelegate: false
    onUseListDelegateChanged: if (picker) picker.useListDelegate = useListDelegate
}
