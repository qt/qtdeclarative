import QtQuick

Item {
    function f() {
        try { insideTry; } catch(catchExpression) { insideCatch; } finally { insideFinally; }
        try { insideTry; } catch(catchExpression) { insideCatch; }
        try { insideTry; } finally { insideFinally; }
    }

}
