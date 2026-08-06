import QtQml

BaseWithInterceptor {
    id: root
    property int trigger: 1
    property alias aliasProp: root.prop
}
