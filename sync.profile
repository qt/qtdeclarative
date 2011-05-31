%modules = ( # path to module name map
    "QtDeclarative" => "$basedir/src/declarative",
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
    "QtQuickTest" => "$basedir/modules/qt_qmltest.pri",
);
# Modules and programs, and their dependencies.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - "LATEST_REVISION", to always test against the latest revision.
#   - "LATEST_RELEASE", to always test against the latest public release.
#   - "THIS_REPOSITORY", to indicate that the module is in this repository.
%dependencies = (
    "QtDeclarative" => {
        "QtScript" => "4d15ca64fc7ca81bdadba9fbeb84d4e98a6c0edc",
        "QtSvg" => "1a71611b6ceaf6cdb24ea485a818fc56c956b5f8",
        "QtGui" => "0c637cb07ba3c9b353e7e483a209537485cc4e2a",
        "QtXmlPatterns" => "26edd6852a62aeec49712a53dcc8d4093192301c",
        "QtNetwork" => "0c637cb07ba3c9b353e7e483a209537485cc4e2a",
        "QtSql" => "0c637cb07ba3c9b353e7e483a209537485cc4e2a",
        "QtCore" => "0c637cb07ba3c9b353e7e483a209537485cc4e2a",
    },
    "QtQuickTest" => {
        "QtTest" => "0c637cb07ba3c9b353e7e483a209537485cc4e2a",
        "QtDeclarative" => "THIS_REPOSITORY",
    },
);
