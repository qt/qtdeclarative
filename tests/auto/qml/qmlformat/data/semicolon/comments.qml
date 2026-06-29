import QtQuick

Item {
    Component.onCompleted: {
        const foo = 1
        // comment
        const foo2 = 2
        /* comment */

        // semicolons
        f; // don't remove, ( follows the comment
        (1 + 1)
        f /* don't remove / */ ;(1 + 1)
        f ;/* multiline.
        also / */ (1 + 1)

        f /* can remove*/ ;
        const result = t;
    }
}
