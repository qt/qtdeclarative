import QtQuick 2.0

Item {
    component Base : Item {
        required property string required_now_string
        property string required_later_string

        required property QtObject required_now_object
        property QtObject required_later_object
    }

    component Derived : Base {
        required required_later_string
        required required_later_object
    }

    Derived {
       required_now_string: ""
       required_later_string: ""

       required_now_object: QtObject {}
       required_later_object: QtObject {}
    }
}
