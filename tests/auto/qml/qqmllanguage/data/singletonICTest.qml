import QtQuick 2.0
import "singleton" as MySingleton

Item {
    property MySingleton.SingletonTypeWithIC.IC1 singleton1: MySingleton.SingletonTypeWithIC.IC1 {};
}
