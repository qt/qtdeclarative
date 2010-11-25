%modules = ( # path to module name map
    "QtDeclarative" => "$basedir/src/declarative",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
);
%classnames = (
);
%mastercontent = (
    "gui" => "#include <QtGui/QtGui>\n",
    "script" => "#include <QtScript/QtScript>\n",
    "network" => "#include <QtNetwork/QtNetwork>\n",
);
%modulepris = (
    "QtDeclarative" => "$basedir/modules/qt_declarative.pri",
);
