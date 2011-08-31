TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += declarative qtquick1 plugins

# ### refactor: port properly
# contains(QT_CONFIG, qmltest): SUBDIRS += qmltest

SUBDIRS += imports

