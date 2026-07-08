// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>
#include <QtQml/private/qqmlexpression_p.h>
#include <QQmlComponent>

#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtranslator.h>
#include <QSignalSpy>

#include <QtQuickTestUtils/private/qmlutils_p.h>

Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<QVariantHash>)

#define RUNEVAL(object, string) \
    QVERIFY(QMetaObject::invokeMethod(object, "runEval", Q_ARG(QVariant, QString(string))));

inline QVariant runexpr(QQmlEngine *engine, const QString &str)
{
    QQmlExpression expr(engine->rootContext(), nullptr, str);
    return expr.evaluate();
}

#define RUNEXPR(string) runexpr(&engine, QString(string))

static bool isValidErrorMessage(const QString &msg, bool dynamicRoleTest)
{
    bool valid = true;

    if (msg.isEmpty()) {
        valid = false;
    } else if (dynamicRoleTest) {
        if (msg.contains("Can't assign to existing role") || msg.contains("Can't create role for unsupported data type"))
            valid = false;
    }

    return valid;
}

class tst_qqmllistmodelworkerscript : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmllistmodelworkerscript()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {
        qRegisterMetaType<QList<int> >();
    }

private:
    int roleFromName(const QQmlListModel *model, const QString &roleName);
    std::unique_ptr<QQuickItem> createWorkerTest(QQmlEngine *eng, QQmlComponent *component, QQmlListModel *model);
    void waitForWorker(QQuickItem *item);

    static bool compareVariantList(const QVariantList &testList, QVariant object);

private slots:
    void dynamic_data();
    void dynamic_worker_data();
    void dynamic_worker();
    void dynamic_worker_sync_data();
    void dynamic_worker_sync();
    void get_data();
    void get_worker();
    void get_worker_data();
    void property_changes_data();
    void property_changes_worker();
    void property_changes_worker_data();
    void worker_sync_data();
    void worker_sync();
    void worker_sync_prepend();
    void worker_remove_element_data();
    void worker_remove_element();
    void worker_remove_list_data();
    void worker_remove_list();
    void dynamic_role_data();
    void dynamic_role();
    void correctMoves();
};

bool tst_qqmllistmodelworkerscript::compareVariantList(const QVariantList &testList, QVariant object)
{
    bool allOk = true;

    QQmlListModel *model = qobject_cast<QQmlListModel *>(object.value<QObject *>());
    if (model == nullptr)
        return false;

    if (model->count() != testList.size())
        return false;

    for (int i=0 ; i < testList.size() ; ++i) {
        const QVariant &testVariant = testList.at(i);
        if (testVariant.typeId() != QMetaType::QVariantMap)
            return false;
        const QVariantMap &map = testVariant.toMap();

        const QHash<int, QByteArray> roleNames = model->roleNames();

        QVariantMap::const_iterator it = map.begin();
        QVariantMap::const_iterator end = map.end();

        while (it != end) {
            const QString &testKey = it.key();
            const QVariant &testData = it.value();

            int roleIndex = roleNames.key(testKey.toUtf8(), -1);
            if (roleIndex == -1)
                return false;

            const QVariant &modelData = model->data(model->index(i, 0, QModelIndex()), roleIndex);

            if (testData.typeId() == QMetaType::QVariantList) {
                const QVariantList &subList = testData.toList();
                allOk = allOk && compareVariantList(subList, modelData);
            } else {
                allOk = allOk && (testData == modelData);
            }

            ++it;
        }
    }

    return allOk;
}

int tst_qqmllistmodelworkerscript::roleFromName(const QQmlListModel *model, const QString &roleName)
{
    return model->roleNames().key(roleName.toUtf8(), -1);
}

std::unique_ptr<QQuickItem> tst_qqmllistmodelworkerscript::createWorkerTest(QQmlEngine *eng, QQmlComponent *component, QQmlListModel *model)
{
    std::unique_ptr<QQuickItem> item { qobject_cast<QQuickItem*>(component->create()) };
    QQmlEngine::setContextForObject(model, eng->rootContext());
    if (item)
        item->setProperty("model", QVariant::fromValue(model));
    return item;
}

void tst_qqmllistmodelworkerscript::waitForWorker(QQuickItem *item)
{
    QQmlProperty prop(item, "done");
    QVERIFY(prop.isValid());
    if (prop.read().toBool())
        return; // already finished

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    QVERIFY(prop.connectNotifySignal(&loop, SLOT(quit())));
    timer.start(10000);
    loop.exec();
    QVERIFY(timer.isActive());
    QVERIFY(prop.read().toBool());
}

void tst_qqmllistmodelworkerscript::dynamic_data()
{
    QTest::addColumn<QString>("script");
    QTest::addColumn<int>("result");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<bool>("dynamicRoles");

    for (const bool dr : { false, true }) {
        const auto t = [dr]() {
            return dr ? "_dr" : "";
        };

        // Simple flat model
        QTest::addRow("count%s", t()) << "count" << 0 << "" << dr;

        QTest::addRow("get1%s", t()) << "{get(0) === undefined}" << 1 << "" << dr;
        QTest::addRow("get2%s", t()) << "{get(-1) === undefined}" << 1 << "" << dr;
        QTest::addRow("get3%s", t()) << "{append({'foo':123});get(0) != undefined}" << 1 << "" << dr;
        QTest::addRow("get4%s", t()) << "{append({'foo':123});get(0).foo}" << 123 << "" << dr;
        QTest::addRow("get-modify1%s", t()) << "{append({'foo':123,'bar':456});get(0).foo = 333;get(0).foo}" << 333 << "" << dr;
        QTest::addRow("get-modify2%s", t()) << "{append({'z':1});append({'foo':123,'bar':456});get(1).bar = 999;get(1).bar}" << 999 << "" << dr;
        QTest::addRow("get-set%s", t()) << "{append({'foo':123});get(0).foo;setProperty(0, 'foo', 999);get(0).foo}" << 999 << "" << dr;

        QTest::addRow("append1%s", t()) << "{append({'foo':123});count}" << 1 << "" << dr;
        QTest::addRow("append2%s", t()) << "{append({'foo':123,'bar':456});count}" << 1 << "" << dr;
        QTest::addRow("append3a%s", t()) << "{append({'foo':123});append({'foo':456});get(0).foo}" << 123 << "" << dr;
        QTest::addRow("append3b%s", t()) << "{append({'foo':123});append({'foo':456});get(1).foo}" << 456 << "" << dr;
        QTest::addRow("append4a%s", t()) << "{append(123)}" << 0 << "<Unknown File>: QML ListModel: append: value is not an object" << dr;
        QTest::addRow("append4b%s", t()) << "{append([{'foo':123},{'foo':456},{'foo':789}]);count}" << 3 << "" << dr;
        QTest::addRow("append4c%s", t()) << "{append([{'foo':123},{'foo':456},{'foo':789}]);get(1).foo}" << 456 << "" << dr;

        QTest::addRow("clear1%s", t()) << "{append({'foo':456});clear();count}" << 0 << "" << dr;
        QTest::addRow("clear2%s", t()) << "{append({'foo':123});append({'foo':456});clear();count}" << 0 << "" << dr;
        QTest::addRow("clear3%s", t()) << "{append({'foo':123});clear()}" << 0 << "" << dr;

        QTest::addRow("remove1%s", t()) << "{append({'foo':123});remove(0);count}" << 0 << "" << dr;
        QTest::addRow("remove2a%s", t()) << "{append({'foo':123});append({'foo':456});remove(0);count}" << 1 << "" << dr;
        QTest::addRow("remove2b%s", t()) << "{append({'foo':123});append({'foo':456});remove(0);get(0).foo}" << 456 << "" << dr;
        QTest::addRow("remove2c%s", t()) << "{append({'foo':123});append({'foo':456});remove(1);get(0).foo}" << 123 << "" << dr;
        QTest::addRow("remove3%s", t()) << "{append({'foo':123});remove(0)}" << 0 << "" << dr;
        QTest::addRow("remove3a%s", t()) << "{append({'foo':123});remove(-1);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [-1 - 0] out of range [0 - 1]" << dr;
        QTest::addRow("remove4a%s", t()) << "{remove(0)}" << 0 << "<Unknown File>: QML ListModel: remove: indices [0 - 1] out of range [0 - 0]" << dr;
        QTest::addRow("remove4b%s", t()) << "{append({'foo':123});remove(0);remove(0);count}" << 0 << "<Unknown File>: QML ListModel: remove: indices [0 - 1] out of range [0 - 0]" << dr;
        QTest::addRow("remove4c%s", t()) << "{append({'foo':123});remove(1);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [1 - 2] out of range [0 - 1]" << dr;
        QTest::addRow("remove5a%s", t()) << "{append({'foo':123});append({'foo':456});remove(0,2);count}" << 0 << "" << dr;
        QTest::addRow("remove5b%s", t()) << "{append({'foo':123});append({'foo':456});remove(0,1);count}" << 1 << "" << dr;
        QTest::addRow("remove5c%s", t()) << "{append({'foo':123});append({'foo':456});remove(1,1);count}" << 1 << "" << dr;
        QTest::addRow("remove5d%s", t()) << "{append({'foo':123});append({'foo':456});remove(0,1);get(0).foo}" << 456 << "" << dr;
        QTest::addRow("remove5e%s", t()) << "{append({'foo':123});append({'foo':456});remove(1,1);get(0).foo}" << 123 << "" << dr;
        QTest::addRow("remove5f%s", t()) << "{append({'foo':123});append({'foo':456});append({'foo':789});remove(0,1);remove(1,1);get(0).foo}" << 456 << "" << dr;
        QTest::addRow("remove6a%s", t()) << "{remove();count}" << 0 << "<Unknown File>: QML ListModel: remove: incorrect number of arguments" << dr;
        QTest::addRow("remove6b%s", t()) << "{remove(1,2,3);count}" << 0 << "<Unknown File>: QML ListModel: remove: incorrect number of arguments" << dr;
        QTest::addRow("remove7a%s", t()) << "{append({'foo':123});remove(0,0);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [0 - 0] out of range [0 - 1]" << dr;
        QTest::addRow("remove7b%s", t()) << "{append({'foo':123});remove(0,-1);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [0 - -1] out of range [0 - 1]" << dr;

        QTest::addRow("insert1%s", t()) << "{insert(0,{'foo':123});count}" << 1 << "" << dr;
        QTest::addRow("insert2%s", t()) << "{insert(1,{'foo':123});count}" << 0 << "<Unknown File>: QML ListModel: insert: index 1 out of range" << dr;
        QTest::addRow("insert3a%s", t()) << "{append({'foo':123});insert(1,{'foo':456});count}" << 2 << "" << dr;
        QTest::addRow("insert3b%s", t()) << "{append({'foo':123});insert(1,{'foo':456});get(0).foo}" << 123 << "" << dr;
        QTest::addRow("insert3c%s", t()) << "{append({'foo':123});insert(1,{'foo':456});get(1).foo}" << 456 << "" << dr;
        QTest::addRow("insert3d%s", t()) << "{append({'foo':123});insert(0,{'foo':456});get(0).foo}" << 456 << "" << dr;
        QTest::addRow("insert3e%s", t()) << "{append({'foo':123});insert(0,{'foo':456});get(1).foo}" << 123 << "" << dr;
        QTest::addRow("insert4%s", t()) << "{append({'foo':123});insert(-1,{'foo':456});count}" << 1 << "<Unknown File>: QML ListModel: insert: index -1 out of range" << dr;
        QTest::addRow("insert5a%s", t()) << "{insert(0,123)}" << 0 << "<Unknown File>: QML ListModel: insert: value is not an object" << dr;
        QTest::addRow("insert5b%s", t()) << "{insert(0,[{'foo':11},{'foo':22},{'foo':33}]);count}" << 3 << "" << dr;
        QTest::addRow("insert5c%s", t()) << "{insert(0,[{'foo':11},{'foo':22},{'foo':33}]);get(2).foo}" << 33 << "" << dr;

        QTest::addRow("set1%s", t()) << "{append({'foo':123});set(0,{'foo':456});count}" << 1 << "" << dr;
        QTest::addRow("set2%s", t()) << "{append({'foo':123});set(0,{'foo':456});get(0).foo}" << 456 << "" << dr;
        QTest::addRow("set3a%s", t()) << "{append({'foo':123,'bar':456});set(0,{'foo':999});get(0).foo}" << 999 << "" << dr;
        QTest::addRow("set3b%s", t()) << "{append({'foo':123,'bar':456});set(0,{'foo':999});get(0).bar}" << 456 << "" << dr;
        QTest::addRow("set4a%s", t()) << "{set(0,{'foo':456});count}" << 1 << "" << dr;
        QTest::addRow("set4c%s", t()) << "{set(-1,{'foo':456})}" << 0 << "<Unknown File>: QML ListModel: set: index -1 out of range" << dr;
        QTest::addRow("set5a%s", t()) << "{append({'foo':123,'bar':456});set(0,123);count}" << 1 << "<Unknown File>: QML ListModel: set: value is not an object" << dr;
        QTest::addRow("set5b%s", t()) << "{append({'foo':123,'bar':456});set(0,[1,2,3]);count}" << 1 << "" << dr;
        QTest::addRow("set6%s", t()) << "{append({'foo':123});set(1,{'foo':456});count}" << 2 << "" << dr;

        QTest::addRow("setprop1%s", t()) << "{append({'foo':123});setProperty(0,'foo',456);count}" << 1 << "" << dr;
        QTest::addRow("setprop2%s", t()) << "{append({'foo':123});setProperty(0,'foo',456);get(0).foo}" << 456 << "" << dr;
        QTest::addRow("setprop3a%s", t()) << "{append({'foo':123,'bar':456});setProperty(0,'foo',999);get(0).foo}" << 999 << "" << dr;
        QTest::addRow("setprop3b%s", t()) << "{append({'foo':123,'bar':456});setProperty(0,'foo',999);get(0).bar}" << 456 << "" << dr;
        QTest::addRow("setprop4a%s", t()) << "{setProperty(0,'foo',456)}" << 0 << "<Unknown File>: QML ListModel: set: index 0 out of range" << dr;
        QTest::addRow("setprop4b%s", t()) << "{setProperty(-1,'foo',456)}" << 0 << "<Unknown File>: QML ListModel: set: index -1 out of range" << dr;
        QTest::addRow("setprop4c%s", t()) << "{append({'foo':123,'bar':456});setProperty(1,'foo',456);count}" << 1 << "<Unknown File>: QML ListModel: set: index 1 out of range" << dr;
        QTest::addRow("setprop5%s", t()) << "{append({'foo':123,'bar':456});append({'foo':111});setProperty(1,'bar',222);get(1).bar}" << 222 << "" << dr;

        QTest::addRow("move1a%s", t()) << "{append({'foo':123});append({'foo':456});move(0,1,1);count}" << 2 << "" << dr;
        QTest::addRow("move1b%s", t()) << "{append({'foo':123});append({'foo':456});move(0,1,1);get(0).foo}" << 456 << "" << dr;
        QTest::addRow("move1c%s", t()) << "{append({'foo':123});append({'foo':456});move(0,1,1);get(1).foo}" << 123 << "" << dr;
        QTest::addRow("move1d%s", t()) << "{append({'foo':123});append({'foo':456});move(1,0,1);get(0).foo}" << 456 << "" << dr;
        QTest::addRow("move1e%s", t()) << "{append({'foo':123});append({'foo':456});move(1,0,1);get(1).foo}" << 123 << "" << dr;
        QTest::addRow("move2a%s", t()) << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);count}" << 3 << "" << dr;
        QTest::addRow("move2b%s", t()) << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(0).foo}" << 789 << "" << dr;
        QTest::addRow("move2c%s", t()) << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(1).foo}" << 123 << "" << dr;
        QTest::addRow("move2d%s", t()) << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(2).foo}" << 456 << "" << dr;
        QTest::addRow("move3a%s", t()) << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,0,3);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;
        QTest::addRow("move3b%s", t()) << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,-1,1);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;
        QTest::addRow("move3c%s", t()) << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,0,-1);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;
        QTest::addRow("move3d%s", t()) << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,3,1);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;

        QTest::addRow("large1%s", t()) << "{append({'a':1,'b':2,'c':3,'d':4,'e':5,'f':6,'g':7,'h':8});get(0).h}" << 8 << "" << dr;

        QTest::addRow("datatypes1%s", t()) << "{append({'a':1});append({'a':'string'});}" << 0 << "<Unknown File>: Can't assign to existing role 'a' of different type [String -> Number]" << dr;

        QTest::addRow("null%s", t()) << "{append({'a':null});}" << 0 << "" << dr;
        QTest::addRow("setNull%s", t()) << "{append({'a':1});set(0, {'a':null});}" << 0 << "" << dr;
        QTest::addRow("setString%s", t()) << "{append({'a':'hello'});set(0, {'a':'world'});get(0).a == 'world'}" << 1 << "" << dr;
        QTest::addRow("setInt%s", t()) << "{append({'a':5});set(0, {'a':10});get(0).a}" << 10 << "" << dr;
        QTest::addRow("setNumber%s", t()) << "{append({'a':6});set(0, {'a':5.5});get(0).a < 5.6}" << 1 << "" << dr;
        QTest::addRow("badType0%s", t()) << "{append({'a':'hello'});set(0, {'a':1});}" << 0 << "<Unknown File>: Can't assign to existing role 'a' of different type [Number -> String]" << dr;
        QTest::addRow("invalidInsert0%s", t()) << "{insert(0);}" << 0 << "<Unknown File>: QML ListModel: insert: value is not an object" << dr;
        QTest::addRow("invalidAppend0%s", t()) << "{append();}" << 0 << "<Unknown File>: QML ListModel: append: value is not an object" << dr;
        QTest::addRow("invalidInsert1%s", t()) << "{insert(0, 34);}" << 0 << "<Unknown File>: QML ListModel: insert: value is not an object" << dr;
        QTest::addRow("invalidAppend1%s", t()) << "{append(37);}" << 0 << "<Unknown File>: QML ListModel: append: value is not an object" << dr;

        // QObjects
        QTest::addRow("qobject0%s", t()) << "{append({'a':dummyItem0});}" << 0 << "" << dr;
        QTest::addRow("qobject1%s", t()) << "{append({'a':dummyItem0});set(0,{'a':dummyItem1});get(0).a == dummyItem1;}" << 1 << "" << dr;
        QTest::addRow("qobject2%s", t()) << "{append({'a':dummyItem0});get(0).a == dummyItem0;}" << 1 << "" << dr;
        QTest::addRow("qobject3%s", t()) << "{append({'a':dummyItem0});append({'b':1});}" << 0 << "" << dr;

        // JS objects
        QTest::addRow("js1%s", t()) << "{append({'foo':{'prop':1}});count}" << 1 << "" << dr;
        QTest::addRow("js2%s", t()) << "{append({'foo':{'prop':27}});get(0).foo.prop}" << 27 << "" << dr;
        QTest::addRow("js3%s", t()) << "{append({'foo':{'prop':27}});append({'bar':1});count}" << 2 << "" << dr;
        QTest::addRow("js4%s", t()) << "{append({'foo':{'prop':27}});append({'bar':1});set(0, {'foo':{'prop':28}});get(0).foo.prop}" << 28 << "" << dr;
        QTest::addRow("js5%s", t()) << "{append({'foo':{'prop':27}});append({'bar':1});set(1, {'foo':{'prop':33}});get(1).foo.prop}" << 33 << "" << dr;
        QTest::addRow("js6%s", t()) << "{append({'foo':{'prop':27}});clear();count}" << 0 << "" << dr;
        QTest::addRow("js7%s", t()) << "{append({'foo':{'prop':27}});set(0, {'foo':null});count}" << 1 << "" << dr;
        QTest::addRow("js8%s", t()) << "{append({'foo':{'prop':27}});set(0, {'foo':{'prop2':31}});get(0).foo.prop2}" << 31 << "" << dr;

        // Nested models
        QTest::addRow("nested-append1%s", t()) << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});count}" << 1 << "" << dr;
        QTest::addRow("nested-append2%s", t()) << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});get(0).bars.get(1).a}" << 2 << "" << dr;
        QTest::addRow("nested-append3%s", t()) << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});get(0).bars.append({'a':4});get(0).bars.get(3).a}" << 4 << "" << dr;

        QTest::addRow("nested-insert%s", t()) << "{append({'foo':123});insert(0,{'bars':[{'a':1},{'b':2},{'c':3}]});get(0).bars.get(0).a}" << 1 << "" << dr;
        QTest::addRow("nested-set%s", t()) << "{append({'foo':[{'x':1}]});set(0,{'foo':[{'x':123}]});get(0).foo.get(0).x}" << 123 << "" << dr;

        QTest::addRow("nested-count%s", t()) << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]}); get(0).bars.count}" << 3 << "" << dr;
        QTest::addRow("nested-clear%s", t()) << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]}); get(0).bars.clear(); get(0).bars.count}" << 0 << "" << dr;
    }
}

void tst_qqmllistmodelworkerscript::dynamic_worker_data()
{
    dynamic_data();
}

void tst_qqmllistmodelworkerscript::dynamic_worker()
{
    QFETCH(QString, script);
    QFETCH(int, result);
    QFETCH(QString, warning);
    QFETCH(bool, dynamicRoles);

    if (QByteArray(QTest::currentDataTag()).startsWith("qobject"))
        return;

    // This is same as dynamic() except it applies the test to a ListModel called
    // from a WorkerScript.

    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("model.qml"));
    std::unique_ptr<QQuickItem> item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item);

    QSignalSpy spyCount(&model, SIGNAL(countChanged()));

    if (script[0] == QLatin1Char('{') && script[script.size()-1] == QLatin1Char('}'))
        script = script.mid(1, script.size() - 2);
    QVariantList operations;
    const QStringList statements = script.split(';');
    for (const QString &s : statements) {
        if (!s.isEmpty())
            operations << s;
    }

    if (isValidErrorMessage(warning, dynamicRoles))
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());

    QVERIFY(QMetaObject::invokeMethod(item.get(), "evalExpressionViaWorker",
            Q_ARG(QVariant, operations)));
    waitForWorker(item.get());
    QCOMPARE(QQmlProperty(item.get(), "result").read().toInt(), result);

    if (model.count() > 0)
        QVERIFY(spyCount.size() > 0);

    item.reset();
    qApp->processEvents();
}

void tst_qqmllistmodelworkerscript::dynamic_worker_sync_data()
{
    dynamic_data();
}

void tst_qqmllistmodelworkerscript::dynamic_worker_sync()
{
    QFETCH(QString, script);
    QFETCH(int, result);
    QFETCH(QString, warning);
    QFETCH(bool, dynamicRoles);

    if (QByteArray(QTest::currentDataTag()).startsWith("qobject"))
        return;

    // This is the same as dynamic_worker() except that it executes a set of list operations
    // from the worker script, calls sync(), and tests the changes are reflected in the
    // list in the main thread

    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("model.qml"));
    std::unique_ptr<QQuickItem> item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item);

    if (script[0] == QLatin1Char('{') && script[script.size()-1] == QLatin1Char('}'))
        script = script.mid(1, script.size() - 2);
    QVariantList operations;
    const QStringList statements = script.split(';');
    for (const QString &s : statements) {
        if (!s.isEmpty())
            operations << s;
    }

    if (isValidErrorMessage(warning, dynamicRoles))
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());

    // execute a set of commands on the worker list model, then check the
    // changes are reflected in the list model in the main thread
    QVERIFY(QMetaObject::invokeMethod(item.get(), "evalExpressionViaWorker",
            Q_ARG(QVariant, operations.mid(0, operations.size()-1))));
    waitForWorker(item.get());

    QQmlExpression e(eng.rootContext(), &model, operations.last().toString());
    QCOMPARE(e.evaluate().toInt(), result);

    item.reset();
    qApp->processEvents();
}

void tst_qqmllistmodelworkerscript::get_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("roleName");
    QTest::addColumn<QVariant>("roleValue");
    QTest::addColumn<bool>("dynamicRoles");

    for (int i =0; i < 2; ++i) {
        bool dr = (i != 0);

        QTest::newRow("simple value") << "get(0).roleA = 500" << 0 << "roleA" << QVariant(500) << dr;
        QTest::newRow("simple value 2") << "get(1).roleB = 500" << 1 << "roleB" << QVariant(500) << dr;

        QVariantMap map;
        QVariantList list;
        map.clear(); map["a"] = 50; map["b"] = 500;
        list << map;
        map.clear(); map["c"] = 1000;
        list << map;
        QTest::newRow("list of objects") << "get(2).roleD = [{'a': 50, 'b': 500}, {'c': 1000}]" << 2 << "roleD" << QVariant::fromValue(list) << dr;
    }
}

void tst_qqmllistmodelworkerscript::get_worker()
{
    QFETCH(QString, expression);
    QFETCH(int, index);
    QFETCH(QString, roleName);
    QFETCH(QVariant, roleValue);
    QFETCH(bool, dynamicRoles);

    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("model.qml"));
    std::unique_ptr<QQuickItem> item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item);

    // Add some values like get() test
    RUNEVAL(item.get(), "model.append({roleA: 100})");
    RUNEVAL(item.get(), "model.append({roleA: 200, roleB: 400})");
    RUNEVAL(item.get(), "model.append({roleA: 200, roleB: 400})");
    RUNEVAL(item.get(), "model.append({roleC: {} })");
    RUNEVAL(item.get(), "model.append({roleD: [ { a:1, b:2 }, { c: 3 } ] })");

    int role = roleFromName(&model, roleName);
    QVERIFY(role >= 0);

    QSignalSpy spy(&model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QList<int>)));

    // in the worker thread, change the model data and call sync()
    QVERIFY(QMetaObject::invokeMethod(item.get(), "evalExpressionViaWorker",
            Q_ARG(QVariant, QStringList(expression))));
    waitForWorker(item.get());

    // see if we receive the model changes in the main thread's model
    if (roleValue.typeId() == QMetaType::QVariantList) {
        const QVariantList &list = roleValue.toList();
        QVERIFY(compareVariantList(list, model.data(index, role)));
    } else {
        QCOMPARE(model.data(index, role), roleValue);
    }

    QCOMPARE(spy.size(), 1);

    QList<QVariant> spyResult = spy.takeFirst();
    QCOMPARE(spyResult.at(0).value<QModelIndex>(), model.index(index, 0, QModelIndex()));
    QCOMPARE(spyResult.at(1).value<QModelIndex>(), model.index(index, 0, QModelIndex()));  // only 1 item is modified at a time
    QVERIFY(spyResult.at(2).value<QList<int> >().contains(role));
}

void tst_qqmllistmodelworkerscript::get_worker_data()
{
    get_data();
}

void tst_qqmllistmodelworkerscript::property_changes_data()
{
    QTest::addColumn<QString>("script_setup");
    QTest::addColumn<QString>("script_change");
    QTest::addColumn<QString>("roleName");
    QTest::addColumn<int>("listIndex");
    QTest::addColumn<bool>("itemsChanged");
    QTest::addColumn<QString>("testExpression");
    QTest::addColumn<bool>("dynamicRoles");

    for (int i=1 ; i < 2 ; ++i) {
        bool dr = (i != 0);

        QTest::newRow("set: plain") << "append({'a':123, 'b':456, 'c':789});" << "set(0,{'b':123});"
                << "b" << 0 << true << "get(0).b == 123" << dr;
        QTest::newRow("setProperty: plain") << "append({'a':123, 'b':456, 'c':789});" << "setProperty(0, 'b', 123);"
                << "b" << 0 << true << "get(0).b == 123" << dr;

        QTest::newRow("set: plain, no changes") << "append({'a':123, 'b':456, 'c':789});" << "set(0,{'b':456});"
                << "b" << 0 << false << "get(0).b == 456" << dr;
        QTest::newRow("setProperty: plain, no changes") << "append({'a':123, 'b':456, 'c':789});" << "setProperty(0, 'b', 456);"
                << "b" << 0 << false << "get(0).b == 456" << dr;

        QTest::newRow("set: inserted item")
                << "{append({'a':123, 'b':456, 'c':789}); get(0); insert(0, {'a':0, 'b':0, 'c':0});}"
                << "set(1, {'a':456});"
                << "a" << 1 << true << "get(1).a == 456" << dr;
        QTest::newRow("setProperty: inserted item")
                << "{append({'a':123, 'b':456, 'c':789}); get(0); insert(0, {'a':0, 'b':0, 'c':0});}"
                << "setProperty(1, 'a', 456);"
                << "a" << 1 << true << "get(1).a == 456" << dr;
        QTest::newRow("get: inserted item")
                << "{append({'a':123, 'b':456, 'c':789}); get(0); insert(0, {'a':0, 'b':0, 'c':0});}"
                << "get(1).a = 456;"
                << "a" << 1 << true << "get(1).a == 456" << dr;
        QTest::newRow("set: removed item")
                << "{append({'a':0, 'b':0, 'c':0}); append({'a':123, 'b':456, 'c':789}); get(1); remove(0);}"
                << "set(0, {'a':456});"
                << "a" << 0 << true << "get(0).a == 456" << dr;
        QTest::newRow("setProperty: removed item")
                << "{append({'a':0, 'b':0, 'c':0}); append({'a':123, 'b':456, 'c':789}); get(1); remove(0);}"
                << "setProperty(0, 'a', 456);"
                << "a" << 0 << true << "get(0).a == 456" << dr;
        QTest::newRow("get: removed item")
                << "{append({'a':0, 'b':0, 'c':0}); append({'a':123, 'b':456, 'c':789}); get(1); remove(0);}"
                << "get(0).a = 456;"
                << "a" << 0 << true << "get(0).a == 456" << dr;

        // Following tests only call set() since setProperty() only allows plain
        // values, not lists, as the argument.
        // Note that when a list is changed, itemsChanged() is currently always
        // emitted regardless of whether it actually changed or not.

        QTest::newRow("nested-set: list, new size") << "append({'a':123, 'b':[{'a':1},{'a':2},{'a':3}], 'c':789});" << "set(0,{'b':[{'a':1},{'a':2}]});"
                << "b" << 0 << true << "get(0).b.get(0).a == 1 && get(0).b.get(1).a == 2" << dr;

        QTest::newRow("nested-set: list, empty -> non-empty") << "append({'a':123, 'b':[], 'c':789});" << "set(0,{'b':[{'a':1},{'a':2},{'a':3}]});"
                << "b" << 0 << true << "get(0).b.get(0).a == 1 && get(0).b.get(1).a == 2 && get(0).b.get(2).a == 3" << dr;

        QTest::newRow("nested-set: list, non-empty -> empty") << "append({'a':123, 'b':[{'a':1},{'a':2},{'a':3}], 'c':789});" << "set(0,{'b':[]});"
                << "b" << 0 << true << "get(0).b.count == 0" << dr;

        QTest::newRow("nested-set: list, same size, different values") << "append({'a':123, 'b':[{'a':1},{'a':2},{'a':3}], 'c':789});" << "set(0,{'b':[{'a':1},{'a':222},{'a':3}]});"
                << "b" << 0 << true << "get(0).b.get(0).a == 1 && get(0).b.get(1).a == 222 && get(0).b.get(2).a == 3" << dr;

        QTest::newRow("nested-set: list, no changes") << "append({'a':123, 'b':[{'a':1},{'a':2},{'a':3}], 'c':789});" << "set(0,{'b':[{'a':1},{'a':2},{'a':3}]});"
                << "b" << 0 << true << "get(0).b.get(0).a == 1 && get(0).b.get(1).a == 2 && get(0).b.get(2).a == 3" << dr;

        QTest::newRow("nested-set: list, no changes, empty") << "append({'a':123, 'b':[], 'c':789});" << "set(0,{'b':[]});"
                << "b" << 0 << false << "get(0).b.count == 0" << dr;
    }
}

void tst_qqmllistmodelworkerscript::property_changes_worker()
{
    QFETCH(QString, script_setup);
    QFETCH(QString, script_change);
    QFETCH(QString, roleName);
    QFETCH(int, listIndex);
    QFETCH(bool, itemsChanged);
    QFETCH(bool, dynamicRoles);

    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("model.qml"));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    std::unique_ptr<QQuickItem> item = createWorkerTest(&engine, &component, &model);
    QVERIFY(item);

    QQmlExpression expr(engine.rootContext(), &model, script_setup);
    expr.evaluate();
    QVERIFY2(!expr.hasError(), qPrintable(expr.error().toString()));

    QSignalSpy spyItemsChanged(&model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QList<int>)));

    QVERIFY(QMetaObject::invokeMethod(item.get(), "evalExpressionViaWorker",
            Q_ARG(QVariant, QStringList(script_change))));
    waitForWorker(item.get());

    // test itemsChanged() is emitted correctly
    if (itemsChanged) {
        QCOMPARE(spyItemsChanged.size(), 1);
        QCOMPARE(spyItemsChanged.at(0).at(0).value<QModelIndex>(), model.index(listIndex, 0, QModelIndex()));
        QCOMPARE(spyItemsChanged.at(0).at(1).value<QModelIndex>(), model.index(listIndex, 0, QModelIndex()));
    } else {
        QCOMPARE(spyItemsChanged.size(), 0);
    }

    item.reset();
    qApp->processEvents();
}

void tst_qqmllistmodelworkerscript::property_changes_worker_data()
{
    property_changes_data();
}

void tst_qqmllistmodelworkerscript::worker_sync_data()
{
    QTest::addColumn<bool>("dynamicRoles");

    QTest::newRow("staticRoles") << false;
    QTest::newRow("dynamicRoles") << true;
}

void tst_qqmllistmodelworkerscript::worker_sync()
{
    QFETCH(bool, dynamicRoles);

    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("workersync.qml"));
    std::unique_ptr<QQuickItem> item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item);

    QCOMPARE(model.count(), 0);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "addItem0"));

    QCOMPARE(model.count(), 2);
    QVariant childData = model.data(0, 0);
    QQmlListModel *childModel = qobject_cast<QQmlListModel *>(childData.value<QObject *>());
    QVERIFY(childModel);
    QCOMPARE(childModel->count(), 1);

    QSignalSpy spyModelInserted(&model, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy spyChildInserted(childModel, SIGNAL(rowsInserted(QModelIndex,int,int)));

    QVERIFY(QMetaObject::invokeMethod(item.get(), "addItemViaWorker"));
    waitForWorker(item.get());

    QCOMPARE(model.count(), 2);
    QCOMPARE(childModel->count(), 1);
    QCOMPARE(spyModelInserted.size(), 0);
    QCOMPARE(spyChildInserted.size(), 0);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "doSync"));
    waitForWorker(item.get());

    QCOMPARE(model.count(), 2);
    QCOMPARE(childModel->count(), 2);
    QCOMPARE(spyModelInserted.size(), 0);
    QCOMPARE(spyChildInserted.size(), 1);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "addItemViaWorker"));
    waitForWorker(item.get());

    QCOMPARE(model.count(), 2);
    QCOMPARE(childModel->count(), 2);
    QCOMPARE(spyModelInserted.size(), 0);
    QCOMPARE(spyChildInserted.size(), 1);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "doSync"));
    waitForWorker(item.get());

    QCOMPARE(model.count(), 2);
    QCOMPARE(childModel->count(), 3);
    QCOMPARE(spyModelInserted.size(), 0);
    QCOMPARE(spyChildInserted.size(), 2);

    item.reset();
    qApp->processEvents();
}


void tst_qqmllistmodelworkerscript::worker_sync_prepend()
{
    // Inserting in front of existing rows makes ListModel::sync() compute a move whose
    // destination equals the (stale) source row. beginMoveRows() rejects such a no-op move
    // and endMoveRows() must not be called for it.
    QQmlListModel model;
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("workersync.qml"));
    std::unique_ptr<QQuickItem> item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "addItem0"));
    QCOMPARE(model.count(), 2);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "prependViaWorker"));
    waitForWorker(item.get());
    QCOMPARE(model.count(), 2);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "doSync"));
    waitForWorker(item.get());
    QCOMPARE(model.count(), 3);

    item.reset();
    qApp->processEvents();
}

void tst_qqmllistmodelworkerscript::worker_remove_element_data()
{
    worker_sync_data();
}

void tst_qqmllistmodelworkerscript::worker_remove_element()
{
    QFETCH(bool, dynamicRoles);

    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("workerremoveelement.qml"));
    std::unique_ptr<QQuickItem> item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item);

    QSignalSpy spyModelRemoved(&model, SIGNAL(rowsRemoved(QModelIndex,int,int)));

    QCOMPARE(model.count(), 0);
    QCOMPARE(spyModelRemoved.size(), 0);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "addItem"));

    QCOMPARE(model.count(), 1);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "removeItemViaWorker"));
    waitForWorker(item.get());

    QCOMPARE(model.count(), 1);
    QCOMPARE(spyModelRemoved.size(), 0);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "doSync"));
    waitForWorker(item.get());

    QCOMPARE(model.count(), 0);
    QCOMPARE(spyModelRemoved.size(), 1);

    item.reset();
    qApp->processEvents();

    {
        //don't crash if model was deleted earlier
        std::unique_ptr<QQmlListModel> model = std::make_unique<QQmlListModel>();
        model->setDynamicRoles(dynamicRoles);
        QQmlEngine eng;
        QQmlComponent component(&eng, testFileUrl("workerremoveelement.qml"));
        std::unique_ptr<QQuickItem> item = createWorkerTest(&eng, &component, model.get());
        QVERIFY(item);

        QVERIFY(QMetaObject::invokeMethod(item.get(), "addItem"));

        QCOMPARE(model->count(), 1);

        QVERIFY(QMetaObject::invokeMethod(item.get(), "removeItemViaWorker"));
        QVERIFY(QMetaObject::invokeMethod(item.get(), "doSync"));
        model.reset();
        qApp->processEvents(); //must not crash here
        waitForWorker(item.get());
    }
}

void tst_qqmllistmodelworkerscript::worker_remove_list_data()
{
    worker_sync_data();
}

void tst_qqmllistmodelworkerscript::worker_remove_list()
{
    QFETCH(bool, dynamicRoles);

    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("workerremovelist.qml"));
    std::unique_ptr<QQuickItem> item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item);

    QSignalSpy spyModelRemoved(&model, SIGNAL(rowsRemoved(QModelIndex,int,int)));

    QCOMPARE(model.count(), 0);
    QCOMPARE(spyModelRemoved.size(), 0);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "addList"));

    QCOMPARE(model.count(), 1);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "removeListViaWorker"));
    waitForWorker(item.get());

    QCOMPARE(model.count(), 1);
    QCOMPARE(spyModelRemoved.size(), 0);

    QVERIFY(QMetaObject::invokeMethod(item.get(), "doSync"));
    waitForWorker(item.get());

    QCOMPARE(model.count(), 0);
    QCOMPARE(spyModelRemoved.size(), 1);

    item.reset();
    qApp->processEvents();
}

void tst_qqmllistmodelworkerscript::dynamic_role_data()
{
    QTest::addColumn<QString>("preamble");
    QTest::addColumn<QString>("script");
    QTest::addColumn<int>("result");

    QTest::newRow("sync1") << "{append({'a':[{'b':1},{'b':2}]})}" << "{get(0).a = 'string';count}" << 1;
}

void tst_qqmllistmodelworkerscript::dynamic_role()
{
    QFETCH(QString, preamble);
    QFETCH(QString, script);
    QFETCH(int, result);

    QQmlListModel model;
    model.setDynamicRoles(true);
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("model.qml"));
    std::unique_ptr<QQuickItem> item = createWorkerTest(&engine, &component, &model);
    QVERIFY(item);

    QQmlExpression preExp(engine.rootContext(), &model, preamble);
    QCOMPARE(preExp.evaluate().toInt(), 0);

    if (script[0] == QLatin1Char('{') && script[script.size()-1] == QLatin1Char('}'))
        script = script.mid(1, script.size() - 2);
    QVariantList operations;
    const QStringList statements = script.split(';');
    for (const QString &s : statements) {
        if (!s.isEmpty())
            operations << s;
    }

    // execute a set of commands on the worker list model, then check the
    // changes are reflected in the list model in the main thread
    QVERIFY(QMetaObject::invokeMethod(item.get(), "evalExpressionViaWorker",
            Q_ARG(QVariant, operations.mid(0, operations.size()-1))));
    waitForWorker(item.get());

    QQmlExpression e(engine.rootContext(), &model, operations.last().toString());
    QCOMPARE(e.evaluate().toInt(), result);

    item.reset();
    qApp->processEvents();
}

void tst_qqmllistmodelworkerscript::correctMoves()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("listmodel_async_sort/main.qml"));
    QScopedPointer<QObject> root {component.create()};
    QVERIFY2(root, qPrintable(component.errorString()));
    bool ok =QMetaObject::invokeMethod(root.get(), "doSort");
    QVERIFY(ok);
    auto check = [&](){
        bool success = false;
        QMetaObject::invokeMethod(root.get(), "verify", Q_RETURN_ARG(bool, success));
        return success;
    };
    QTRY_VERIFY(check());
}

QTEST_MAIN(tst_qqmllistmodelworkerscript)

#include "tst_qqmllistmodelworkerscript.moc"
