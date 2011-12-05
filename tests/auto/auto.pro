TEMPLATE=subdirs
SUBDIRS=\
    declarative \
    qtquick2 \
    particles

# ### refactor: port properly
# contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

SUBDIRS += qmldevtools

!cross_compile:                             SUBDIRS += host.pro
