TEMPLATE=subdirs
SUBDIRS=\
    declarative

# ### refactor: port properly
# contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

!cross_compile:                             SUBDIRS += host.pro
