%modules = ( # path to module name map
    "QtQml" => "$basedir/src/qml",
    "QtQuick" => "$basedir/src/quick",
    "QtQuickTest" => "$basedir/src/qmltest",
    "QtQmlDevTools" => "$basedir/src/qmldevtools",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
    "QtQmlDevTools" => "../qml/qml/parser",
);
%classnames = (
    "qtqmlversion.h" => "QtQmlVersion",
);
%mastercontent = (
    "gui" => "#include <QtGui/QtGui>\n",
    "script" => "#include <QtScript/QtScript>\n",
    "network" => "#include <QtNetwork/QtNetwork>\n",
    "testlib" => "#include <QtTest/QtTest>\n",
);
%modulepris = (
    "QtQml" => "$basedir/modules/qt_qml.pri",
    "QtQuick" => "$basedir/modules/qt_quick.pri",
    "QtQuickTest" => "$basedir/modules/qt_qmltest.pri",
    "QtQmlDevTools" => "$basedir/modules/qt_qmldevtools.pri",
);
%deprecatedheaders = (
);
# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
        "qtbase" => "refs/heads/api_changes",
        "qtxmlpatterns" => "refs/heads/master",
        "qtjsbackend" => "refs/heads/master",
);
