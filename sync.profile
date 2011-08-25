%modules = ( # path to module name map
    "QtDeclarative" => "$basedir/src/declarative",
    "QtQuick1" => "$basedir/src/qtquick1",
    "QtQuickTest" => "$basedir/src/qmltest",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
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
    "QtQuick1" => "$basedir/modules/qt_qtquick1.pri",
    "QtQuickTest" => "$basedir/modules/qt_qmltest.pri",
);
# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
        "qtbase" => "refs/heads/master",
        "qtsvg" => "refs/heads/master",
        "qtxmlpatterns" => "refs/heads/master",
);
