TEMPLATE=subdirs
SUBDIRS=\
    declarative \

!cross_compile:                             SUBDIRS += host.pro
