TEMPLATE = subdirs

module_qtdeclarative_src.subdir = src
module_qtdeclarative_src.target = module-qtdeclarative-src

module_qtdeclarative_tools.subdir = tools
module_qtdeclarative_tools.target = module-qtdeclarative-tools
module_qtdeclarative_tools.depends = module_qtdeclarative_src

module_qtdeclarative_examples.subdir = examples/declarative
module_qtdeclarative_examples.target = module-qtdeclarative-examples
module_qtdeclarative_examples.depends = module_qtdeclarative_src
!contains(QT_BUILD_PARTS,examples) {
    module_qtdeclarative_examples.CONFIG = no_default_target no_default_install
}

module_qtdeclarative_tests.subdir = tests
module_qtdeclarative_tests.target = module-qtdeclarative-tests
module_qtdeclarative_tests.depends = module_qtdeclarative_src
module_qtdeclarative_tests.CONFIG = no_default_install
!contains(QT_BUILD_PARTS,tests):module_qtdeclarative_tests.CONFIG += no_default_target

SUBDIRS += module_qtdeclarative_src \
           module_qtdeclarative_tools \
           module_qtdeclarative_examples \
           module_qtdeclarative_tests \

include(doc/config/qtdeclarative_doc.pri)