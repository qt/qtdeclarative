%modules = ( # path to module name map
    "QtV4" => "$basedir/src",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
    #"QtV4" => "3rdparty/masm;v4;",
);
@allmoduleheadersprivate = (
    "QtV4"
);

# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
        "qtbase" => "refs/heads/dev",
        "qtdeclarative" => "refs/heads/dev",
);
