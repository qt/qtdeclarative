%modules = ( # path to module name map
    "QtDeclarative" => "$basedir/src/declarative",
    "QtQuick" => "$basedir/src/quick",
    "QtQuickTest" => "$basedir/src/qmltest",
    "QtQmlDevTools" => "$basedir/src/qmldevtools",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
    "QtQmlDevTools" => "../declarative/qml/parser",
);
%classnames = (
    "qtdeclarativeversion.h" => "QtDeclarativeVersion",
);
%mastercontent = (
    "gui" => "#include <QtGui/QtGui>\n",
    "script" => "#include <QtScript/QtScript>\n",
    "network" => "#include <QtNetwork/QtNetwork>\n",
    "testlib" => "#include <QtTest/QtTest>\n",
);
%modulepris = (
    "QtDeclarative" => "$basedir/modules/qt_declarative.pri",
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
        "qtbase" => "95c5be8bc1c8f9ff8e7f95e52a8abd3cd7976ba1",
        "qtxmlpatterns" => "refs/heads/master",
        "qtjsbackend" => "refs/heads/master",
);
