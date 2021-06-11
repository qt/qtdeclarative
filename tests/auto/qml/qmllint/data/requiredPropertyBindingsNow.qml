import QtQuick 2.0

Item {
    component Base : Item {
        required property string required_now_string
        property string required_later_string
    }

    component Derived : Base {
        required required_later_string
    }

    Derived {
       required property string required_defined_here_string
       required_later_string: ""
    }
}
