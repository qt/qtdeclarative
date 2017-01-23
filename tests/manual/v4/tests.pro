TEMPLATE = aux

TESTSCRIPT=$$PWD/test262.py
isEmpty(V4CMD): V4CMD = qmljs

checktarget.target = check
checktarget.commands = python $$TESTSCRIPT --command=$$V4CMD --parallel --with-test-expectations --update-expectations
checktarget.depends = all
QMAKE_EXTRA_TARGETS += checktarget

checkmothtarget.target = check-interpreter
checkmothtarget.commands = python $$TESTSCRIPT --command=\"$$V4CMD --interpret\" --parallel --with-test-expectations
checkmothtarget.depends = all
QMAKE_EXTRA_TARGETS += checkmothtarget

