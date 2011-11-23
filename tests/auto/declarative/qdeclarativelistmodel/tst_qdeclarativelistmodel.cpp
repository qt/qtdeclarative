/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtDeclarative/private/qdeclarativeengine_p.h>
#include <QtDeclarative/private/qdeclarativelistmodel_p.h>
#include <QtDeclarative/private/qdeclarativeexpression_p.h>
#include <QDeclarativeComponent>

#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtranslator.h>
#include <QSignalSpy>

#include "../../shared/util.h"

Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<QVariantHash>)

#define RUNEVAL(object, string) \
    QVERIFY(QMetaObject::invokeMethod(object, "runEval", Q_ARG(QVariant, QString(string))));

inline QVariant runexpr(QDeclarativeEngine *engine, const QString &str)
{
    QDeclarativeExpression expr(engine->rootContext(), 0, str);
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

class tst_qdeclarativelistmodel : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativelistmodel() {}

private:
    int roleFromName(const QDeclarativeListModel *model, const QString &roleName);
    QQuickItem *createWorkerTest(QDeclarativeEngine *eng, QDeclarativeComponent *component, QDeclarativeListModel *model);
    void waitForWorker(QQuickItem *item);

    static bool compareVariantList(const QVariantList &testList, QVariant object);

private slots:
    void static_types();
    void static_types_data();
    void static_i18n();
    void static_i18n_data();
    void static_nestedElements();
    void static_nestedElements_data();
    void dynamic_data();
    void dynamic();
    void dynamic_worker_data();
    void dynamic_worker();
    void dynamic_worker_sync_data();
    void dynamic_worker_sync();
    void enumerate();
    void error_data();
    void error();
    void syncError();
    void get();
    void set_data();
    void set();
    void get_data();
    void get_worker();
    void get_worker_data();
    void get_nested();
    void get_nested_data();
    void crash_model_with_multiple_roles();
    void set_model_cache();
    void property_changes();
    void property_changes_data();
    void property_changes_worker();
    void property_changes_worker_data();
    void clear_data();
    void clear();
    void signal_handlers_data();
    void signal_handlers();
    void worker_sync_data();
    void worker_sync();
    void worker_remove_element_data();
    void worker_remove_element();
    void worker_remove_list_data();
    void worker_remove_list();
    void role_mode_data();
    void role_mode();
    void dynamic_role();
    void dynamic_role_data();
};

bool tst_qdeclarativelistmodel::compareVariantList(const QVariantList &testList, QVariant object)
{
    bool allOk = true;

    QDeclarativeListModel *model = qobject_cast<QDeclarativeListModel *>(object.value<QObject *>());
    if (model == 0)
        return false;

    if (model->count() != testList.count())
        return false;

    for (int i=0 ; i < testList.count() ; ++i) {
        const QVariant &testVariant = testList.at(i);
        if (testVariant.type() != QVariant::Map)
            return false;
        const QVariantMap &map = testVariant.toMap();

        const QList<int> &roles = model->roles();

        QVariantMap::const_iterator it = map.begin();
        QVariantMap::const_iterator end = map.end();

        while (it != end) {
            const QString &testKey = it.key();
            const QVariant &testData = it.value();

            int roleIndex = -1;
            for (int j=0 ; j < roles.count() ; ++j) {
                if (model->toString(roles[j]).compare(testKey) == 0) {
                    roleIndex = j;
                    break;
                }
            }

            if (roleIndex == -1)
                return false;

            const QVariant &modelData = model->data(i, roleIndex);

            if (testData.type() == QVariant::List) {
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

int tst_qdeclarativelistmodel::roleFromName(const QDeclarativeListModel *model, const QString &roleName)
{
    QList<int> roles = model->roles();
    for (int i=0; i<roles.count(); i++) {
        if (model->toString(roles[i]) == roleName)
            return roles[i];
    }
    return -1;
}

QQuickItem *tst_qdeclarativelistmodel::createWorkerTest(QDeclarativeEngine *eng, QDeclarativeComponent *component, QDeclarativeListModel *model)
{
    QQuickItem *item = qobject_cast<QQuickItem*>(component->create());
    QDeclarativeEngine::setContextForObject(model, eng->rootContext());
    if (item)
        item->setProperty("model", qVariantFromValue(model));
    return item;
}

void tst_qdeclarativelistmodel::waitForWorker(QQuickItem *item)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    QDeclarativeProperty prop(item, "done");
    QVERIFY(prop.isValid());
    QVERIFY(prop.connectNotifySignal(&loop, SLOT(quit())));
    timer.start(10000);
    loop.exec();
    QVERIFY(timer.isActive());
}

void tst_qdeclarativelistmodel::static_types_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("error");

    QTest::newRow("string")
        << "ListElement { foo: \"bar\" }"
        << QVariant(QString("bar"))
        << QString();

    QTest::newRow("real")
        << "ListElement { foo: 10.5 }"
        << QVariant(10.5)
        << QString();

    QTest::newRow("real0")
        << "ListElement { foo: 0 }"
        << QVariant(double(0))
        << QString();

    QTest::newRow("bool")
        << "ListElement { foo: false }"
        << QVariant(false)
        << QString();

    QTest::newRow("bool")
        << "ListElement { foo: true }"
        << QVariant(true)
        << QString();

    QTest::newRow("enum")
        << "ListElement { foo: Text.AlignHCenter }"
        << QVariant(double(QQuickText::AlignHCenter))
        << QString();

    QTest::newRow("Qt enum")
        << "ListElement { foo: Qt.AlignBottom }"
        << QVariant(double(Qt::AlignBottom))
        << QString();

    QTest::newRow("role error")
        << "ListElement { foo: 1 } ListElement { foo: 'string' }"
        << QVariant()
        << QString("<Unknown File>: Can't assign to existing role 'foo' of different type [String -> Number]");

    QTest::newRow("list type error")
        << "ListElement { foo: 1 } ListElement { foo: ListElement { bar: 1 } }"
        << QVariant()
        << QString("<Unknown File>: Can't assign to existing role 'foo' of different type [List -> Number]");
}

void tst_qdeclarativelistmodel::static_types()
{
    QFETCH(QString, qml);
    QFETCH(QVariant, value);
    QFETCH(QString, error);

    qml = "import QtQuick 2.0\nItem { property variant test: model.get(0).foo; ListModel { id: model; " + qml + " } }";

    if (!error.isEmpty()) {
        QTest::ignoreMessage(QtWarningMsg, error.toLatin1());
    }

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));

    QVERIFY(!component.isError());

    QObject *obj = component.create();
    QVERIFY(obj != 0);

    if (error.isEmpty()) {
        QVariant actual = obj->property("test");

        QCOMPARE(actual, value);
        QCOMPARE(actual.toString(), value.toString());
    }

    delete obj;
}

void tst_qdeclarativelistmodel::static_i18n_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("error");

    QTest::newRow("QT_TR_NOOP")
        << QString::fromUtf8("ListElement { foo: QT_TR_NOOP(\"na\303\257ve\") }")
        << QVariant(QString::fromUtf8("na\303\257ve"))
        << QString();

    QTest::newRow("QT_TRANSLATE_NOOP")
        << "ListElement { foo: QT_TRANSLATE_NOOP(\"MyListModel\", \"hello\") }"
        << QVariant(QString("hello"))
        << QString();

    QTest::newRow("QT_TRID_NOOP")
        << QString::fromUtf8("ListElement { foo: QT_TRID_NOOP(\"qtn_1st_text\") }")
        << QVariant(QString("qtn_1st_text"))
        << QString();

    QTest::newRow("QT_TR_NOOP extra param")
            << QString::fromUtf8("ListElement { foo: QT_TR_NOOP(\"hello\",\"world\") }")
            << QVariant(QString())
            << QString("ListElement: improperly specified QT_TR_NOOP");

    QTest::newRow("QT_TRANSLATE_NOOP missing params")
        << "ListElement { foo: QT_TRANSLATE_NOOP() }"
        << QVariant(QString())
        << QString("ListElement: improperly specified QT_TRANSLATE_NOOP");

    QTest::newRow("QT_TRID_NOOP missing param")
        << QString::fromUtf8("ListElement { foo: QT_TRID_NOOP() }")
        << QVariant(QString())
        << QString("ListElement: improperly specified QT_TRID_NOOP");
}

void tst_qdeclarativelistmodel::static_i18n()
{
    QFETCH(QString, qml);
    QFETCH(QVariant, value);
    QFETCH(QString, error);

    qml = "import QtQuick 2.0\nItem { property variant test: model.get(0).foo; ListModel { id: model; " + qml + " } }";

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));

    if (!error.isEmpty()) {
        QVERIFY(component.isError());
        QCOMPARE(component.errors().at(0).description(), error);
        return;
    }

    QVERIFY(!component.isError());

    QObject *obj = component.create();
    QVERIFY(obj != 0);

    QVariant actual = obj->property("test");

    QCOMPARE(actual, value);
    QCOMPARE(actual.toString(), value.toString());

    delete obj;
}

void tst_qdeclarativelistmodel::static_nestedElements()
{
    QFETCH(int, elementCount);

    QStringList elements;
    for (int i=0; i<elementCount; i++)
        elements.append("ListElement { a: 1; b: 2 }");
    QString elementsStr = elements.join(",\n") + "\n";

    QString componentStr =
        "import QtQuick 2.0\n"
        "Item {\n"
        "    property variant count: model.get(0).attributes.count\n"
        "    ListModel {\n"
        "        id: model\n"
        "        ListElement {\n"
        "            attributes: [\n";
    componentStr += elementsStr.toUtf8().constData();
    componentStr +=
        "            ]\n"
        "        }\n"
        "    }\n"
        "}";

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toUtf8(), QUrl::fromLocalFile(""));

    QObject *obj = component.create();
    QVERIFY(obj != 0);

    QVariant count = obj->property("count");
    QCOMPARE(count.type(), QVariant::Int);
    QCOMPARE(count.toInt(), elementCount);

    delete obj;
}

void tst_qdeclarativelistmodel::static_nestedElements_data()
{
    QTest::addColumn<int>("elementCount");

    QTest::newRow("0 items") << 0;
    QTest::newRow("1 item") << 1;
    QTest::newRow("2 items") << 2;
    QTest::newRow("many items") << 5;
}

void tst_qdeclarativelistmodel::dynamic_data()
{
    QTest::addColumn<QString>("script");
    QTest::addColumn<int>("result");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<bool>("dynamicRoles");

    for (int i=0 ; i < 2 ; ++i) {
        bool dr = (i != 0);

        // Simple flat model
        QTest::newRow("count") << "count" << 0 << "" << dr;

        QTest::newRow("get1") << "{get(0) === undefined}" << 1 << "" << dr;
        QTest::newRow("get2") << "{get(-1) === undefined}" << 1 << "" << dr;
        QTest::newRow("get3") << "{append({'foo':123});get(0) != undefined}" << 1 << "" << dr;
        QTest::newRow("get4") << "{append({'foo':123});get(0).foo}" << 123 << "" << dr;
        QTest::newRow("get-modify1") << "{append({'foo':123,'bar':456});get(0).foo = 333;get(0).foo}" << 333 << "" << dr;
        QTest::newRow("get-modify2") << "{append({'z':1});append({'foo':123,'bar':456});get(1).bar = 999;get(1).bar}" << 999 << "" << dr;

        QTest::newRow("append1") << "{append({'foo':123});count}" << 1 << "" << dr;
        QTest::newRow("append2") << "{append({'foo':123,'bar':456});count}" << 1 << "" << dr;
        QTest::newRow("append3a") << "{append({'foo':123});append({'foo':456});get(0).foo}" << 123 << "" << dr;
        QTest::newRow("append3b") << "{append({'foo':123});append({'foo':456});get(1).foo}" << 456 << "" << dr;
        QTest::newRow("append4a") << "{append(123)}" << 0 << "<Unknown File>: QML ListModel: append: value is not an object" << dr;
        QTest::newRow("append4b") << "{append([{'foo':123},{'foo':456},{'foo':789}]);count}" << 3 << "" << dr;
        QTest::newRow("append4c") << "{append([{'foo':123},{'foo':456},{'foo':789}]);get(1).foo}" << 456 << "" << dr;

        QTest::newRow("clear1") << "{append({'foo':456});clear();count}" << 0 << "" << dr;
        QTest::newRow("clear2") << "{append({'foo':123});append({'foo':456});clear();count}" << 0 << "" << dr;
        QTest::newRow("clear3") << "{append({'foo':123});clear()}" << 0 << "" << dr;

        QTest::newRow("remove1") << "{append({'foo':123});remove(0);count}" << 0 << "" << dr;
        QTest::newRow("remove2a") << "{append({'foo':123});append({'foo':456});remove(0);count}" << 1 << "" << dr;
        QTest::newRow("remove2b") << "{append({'foo':123});append({'foo':456});remove(0);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("remove2c") << "{append({'foo':123});append({'foo':456});remove(1);get(0).foo}" << 123 << "" << dr;
        QTest::newRow("remove3") << "{append({'foo':123});remove(0)}" << 0 << "" << dr;
        QTest::newRow("remove3a") << "{append({'foo':123});remove(-1);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [-1 - 0] out of range [0 - 1]" << dr;
        QTest::newRow("remove4a") << "{remove(0)}" << 0 << "<Unknown File>: QML ListModel: remove: indices [0 - 1] out of range [0 - 0]" << dr;
        QTest::newRow("remove4b") << "{append({'foo':123});remove(0);remove(0);count}" << 0 << "<Unknown File>: QML ListModel: remove: indices [0 - 1] out of range [0 - 0]" << dr;
        QTest::newRow("remove4c") << "{append({'foo':123});remove(1);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [1 - 2] out of range [0 - 1]" << dr;
        QTest::newRow("remove5a") << "{append({'foo':123});append({'foo':456});remove(0,2);count}" << 0 << "" << dr;
        QTest::newRow("remove5b") << "{append({'foo':123});append({'foo':456});remove(0,1);count}" << 1 << "" << dr;
        QTest::newRow("remove5c") << "{append({'foo':123});append({'foo':456});remove(1,1);count}" << 1 << "" << dr;
        QTest::newRow("remove5d") << "{append({'foo':123});append({'foo':456});remove(0,1);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("remove5e") << "{append({'foo':123});append({'foo':456});remove(1,1);get(0).foo}" << 123 << "" << dr;
        QTest::newRow("remove5f") << "{append({'foo':123});append({'foo':456});append({'foo':789});remove(0,1);remove(1,1);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("remove6a") << "{remove();count}" << 0 << "<Unknown File>: QML ListModel: remove: incorrect number of arguments" << dr;
        QTest::newRow("remove6b") << "{remove(1,2,3);count}" << 0 << "<Unknown File>: QML ListModel: remove: incorrect number of arguments" << dr;
        QTest::newRow("remove7a") << "{append({'foo':123});remove(0,0);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [0 - 0] out of range [0 - 1]" << dr;
        QTest::newRow("remove7b") << "{append({'foo':123});remove(0,-1);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [0 - -1] out of range [0 - 1]" << dr;

        QTest::newRow("insert1") << "{insert(0,{'foo':123});count}" << 1 << "" << dr;
        QTest::newRow("insert2") << "{insert(1,{'foo':123});count}" << 0 << "<Unknown File>: QML ListModel: insert: index 1 out of range" << dr;
        QTest::newRow("insert3a") << "{append({'foo':123});insert(1,{'foo':456});count}" << 2 << "" << dr;
        QTest::newRow("insert3b") << "{append({'foo':123});insert(1,{'foo':456});get(0).foo}" << 123 << "" << dr;
        QTest::newRow("insert3c") << "{append({'foo':123});insert(1,{'foo':456});get(1).foo}" << 456 << "" << dr;
        QTest::newRow("insert3d") << "{append({'foo':123});insert(0,{'foo':456});get(0).foo}" << 456 << "" << dr;
        QTest::newRow("insert3e") << "{append({'foo':123});insert(0,{'foo':456});get(1).foo}" << 123 << "" << dr;
        QTest::newRow("insert4") << "{append({'foo':123});insert(-1,{'foo':456});count}" << 1 << "<Unknown File>: QML ListModel: insert: index -1 out of range" << dr;
        QTest::newRow("insert5a") << "{insert(0,123)}" << 0 << "<Unknown File>: QML ListModel: insert: value is not an object" << dr;
        QTest::newRow("insert5b") << "{insert(0,[{'foo':11},{'foo':22},{'foo':33}]);count}" << 3 << "" << dr;
        QTest::newRow("insert5c") << "{insert(0,[{'foo':11},{'foo':22},{'foo':33}]);get(2).foo}" << 33 << "" << dr;

        QTest::newRow("set1") << "{append({'foo':123});set(0,{'foo':456});count}" << 1 << "" << dr;
        QTest::newRow("set2") << "{append({'foo':123});set(0,{'foo':456});get(0).foo}" << 456 << "" << dr;
        QTest::newRow("set3a") << "{append({'foo':123,'bar':456});set(0,{'foo':999});get(0).foo}" << 999 << "" << dr;
        QTest::newRow("set3b") << "{append({'foo':123,'bar':456});set(0,{'foo':999});get(0).bar}" << 456 << "" << dr;
        QTest::newRow("set4a") << "{set(0,{'foo':456});count}" << 1 << "" << dr;
        QTest::newRow("set4c") << "{set(-1,{'foo':456})}" << 0 << "<Unknown File>: QML ListModel: set: index -1 out of range" << dr;
        QTest::newRow("set5a") << "{append({'foo':123,'bar':456});set(0,123);count}" << 1 << "<Unknown File>: QML ListModel: set: value is not an object" << dr;
        QTest::newRow("set5b") << "{append({'foo':123,'bar':456});set(0,[1,2,3]);count}" << 1 << "<Unknown File>: QML ListModel: set: value is not an object" << dr;
        QTest::newRow("set6") << "{append({'foo':123});set(1,{'foo':456});count}" << 2 << "" << dr;

        QTest::newRow("setprop1") << "{append({'foo':123});setProperty(0,'foo',456);count}" << 1 << "" << dr;
        QTest::newRow("setprop2") << "{append({'foo':123});setProperty(0,'foo',456);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("setprop3a") << "{append({'foo':123,'bar':456});setProperty(0,'foo',999);get(0).foo}" << 999 << "" << dr;
        QTest::newRow("setprop3b") << "{append({'foo':123,'bar':456});setProperty(0,'foo',999);get(0).bar}" << 456 << "" << dr;
        QTest::newRow("setprop4a") << "{setProperty(0,'foo',456)}" << 0 << "<Unknown File>: QML ListModel: set: index 0 out of range" << dr;
        QTest::newRow("setprop4b") << "{setProperty(-1,'foo',456)}" << 0 << "<Unknown File>: QML ListModel: set: index -1 out of range" << dr;
        QTest::newRow("setprop4c") << "{append({'foo':123,'bar':456});setProperty(1,'foo',456);count}" << 1 << "<Unknown File>: QML ListModel: set: index 1 out of range" << dr;
        QTest::newRow("setprop5") << "{append({'foo':123,'bar':456});append({'foo':111});setProperty(1,'bar',222);get(1).bar}" << 222 << "" << dr;

        QTest::newRow("move1a") << "{append({'foo':123});append({'foo':456});move(0,1,1);count}" << 2 << "" << dr;
        QTest::newRow("move1b") << "{append({'foo':123});append({'foo':456});move(0,1,1);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("move1c") << "{append({'foo':123});append({'foo':456});move(0,1,1);get(1).foo}" << 123 << "" << dr;
        QTest::newRow("move1d") << "{append({'foo':123});append({'foo':456});move(1,0,1);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("move1e") << "{append({'foo':123});append({'foo':456});move(1,0,1);get(1).foo}" << 123 << "" << dr;
        QTest::newRow("move2a") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);count}" << 3 << "" << dr;
        QTest::newRow("move2b") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(0).foo}" << 789 << "" << dr;
        QTest::newRow("move2c") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(1).foo}" << 123 << "" << dr;
        QTest::newRow("move2d") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(2).foo}" << 456 << "" << dr;
        QTest::newRow("move3a") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,0,3);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;
        QTest::newRow("move3b") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,-1,1);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;
        QTest::newRow("move3c") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,0,-1);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;
        QTest::newRow("move3d") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,3,1);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;

        QTest::newRow("large1") << "{append({'a':1,'b':2,'c':3,'d':4,'e':5,'f':6,'g':7,'h':8});get(0).h}" << 8 << "" << dr;

        QTest::newRow("datatypes1") << "{append({'a':1});append({'a':'string'});}" << 0 << "<Unknown File>: Can't assign to existing role 'a' of different type [String -> Number]" << dr;

        QTest::newRow("null") << "{append({'a':null});}" << 0 << "" << dr;
        QTest::newRow("setNull") << "{append({'a':1});set(0, {'a':null});}" << 0 << "" << dr;
        QTest::newRow("setString") << "{append({'a':'hello'});set(0, {'a':'world'});get(0).a == 'world'}" << 1 << "" << dr;
        QTest::newRow("setInt") << "{append({'a':5});set(0, {'a':10});get(0).a}" << 10 << "" << dr;
        QTest::newRow("setNumber") << "{append({'a':6});set(0, {'a':5.5});get(0).a < 5.6}" << 1 << "" << dr;
        QTest::newRow("badType0") << "{append({'a':'hello'});set(0, {'a':1});}" << 0 << "<Unknown File>: Can't assign to existing role 'a' of different type [Number -> String]" << dr;
        QTest::newRow("invalidInsert0") << "{insert(0);}" << 0 << "<Unknown File>: QML ListModel: insert: value is not an object" << dr;
        QTest::newRow("invalidAppend0") << "{append();}" << 0 << "<Unknown File>: QML ListModel: append: value is not an object" << dr;
        QTest::newRow("invalidInsert1") << "{insert(0, 34);}" << 0 << "<Unknown File>: QML ListModel: insert: value is not an object" << dr;
        QTest::newRow("invalidAppend1") << "{append(37);}" << 0 << "<Unknown File>: QML ListModel: append: value is not an object" << dr;

        // QObjects
        QTest::newRow("qobject0") << "{append({'a':dummyItem0});}" << 0 << "" << dr;
        QTest::newRow("qobject1") << "{append({'a':dummyItem0});set(0,{'a':dummyItem1});get(0).a == dummyItem1;}" << 1 << "" << dr;
        QTest::newRow("qobject2") << "{append({'a':dummyItem0});get(0).a == dummyItem0;}" << 1 << "" << dr;
        QTest::newRow("qobject3") << "{append({'a':dummyItem0});append({'b':1});}" << 0 << "" << dr;

        // JS objects
        QTest::newRow("js1") << "{append({'foo':{'prop':1}});count}" << 1 << "" << dr;
        QTest::newRow("js2") << "{append({'foo':{'prop':27}});get(0).foo.prop}" << 27 << "" << dr;
        QTest::newRow("js3") << "{append({'foo':{'prop':27}});append({'bar':1});count}" << 2 << "" << dr;
        QTest::newRow("js4") << "{append({'foo':{'prop':27}});append({'bar':1});set(0, {'foo':{'prop':28}});get(0).foo.prop}" << 28 << "" << dr;
        QTest::newRow("js5") << "{append({'foo':{'prop':27}});append({'bar':1});set(1, {'foo':{'prop':33}});get(1).foo.prop}" << 33 << "" << dr;
        QTest::newRow("js6") << "{append({'foo':{'prop':27}});clear();count}" << 0 << "" << dr;
        QTest::newRow("js7") << "{append({'foo':{'prop':27}});set(0, {'foo':null});count}" << 1 << "" << dr;
        QTest::newRow("js8") << "{append({'foo':{'prop':27}});set(0, {'foo':{'prop2':31}});get(0).foo.prop2}" << 31 << "" << dr;

        // Nested models
        QTest::newRow("nested-append1") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});count}" << 1 << "" << dr;
        QTest::newRow("nested-append2") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});get(0).bars.get(1).a}" << 2 << "" << dr;
        QTest::newRow("nested-append3") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});get(0).bars.append({'a':4});get(0).bars.get(3).a}" << 4 << "" << dr;

        QTest::newRow("nested-insert") << "{append({'foo':123});insert(0,{'bars':[{'a':1},{'b':2},{'c':3}]});get(0).bars.get(0).a}" << 1 << "" << dr;
        QTest::newRow("nested-set") << "{append({'foo':[{'x':1}]});set(0,{'foo':[{'x':123}]});get(0).foo.get(0).x}" << 123 << "" << dr;

        QTest::newRow("nested-count") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]}); get(0).bars.count}" << 3 << "" << dr;
        QTest::newRow("nested-clear") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]}); get(0).bars.clear(); get(0).bars.count}" << 0 << "" << dr;
    }
}

void tst_qdeclarativelistmodel::dynamic()
{
    QFETCH(QString, script);
    QFETCH(int, result);
    QFETCH(QString, warning);
    QFETCH(bool, dynamicRoles);

    QQuickItem dummyItem0, dummyItem1;
    QDeclarativeEngine engine;
    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine::setContextForObject(&model,engine.rootContext());
    engine.rootContext()->setContextObject(&model);
    engine.rootContext()->setContextProperty("dummyItem0", QVariant::fromValue(&dummyItem0));
    engine.rootContext()->setContextProperty("dummyItem1", QVariant::fromValue(&dummyItem1));
    QDeclarativeExpression e(engine.rootContext(), &model, script);
    if (isValidErrorMessage(warning, dynamicRoles))
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());

    QSignalSpy spyCount(&model, SIGNAL(countChanged()));

    int actual = e.evaluate().toInt();
    if (e.hasError())
        qDebug() << e.error(); // errors not expected

    QCOMPARE(actual,result);

    if (model.count() > 0)
        QVERIFY(spyCount.count() > 0);
}

void tst_qdeclarativelistmodel::dynamic_worker_data()
{
    dynamic_data();
}

void tst_qdeclarativelistmodel::dynamic_worker()
{
    QFETCH(QString, script);
    QFETCH(int, result);
    QFETCH(QString, warning);
    QFETCH(bool, dynamicRoles);

    if (QByteArray(QTest::currentDataTag()).startsWith("qobject"))
        return;

    // This is same as dynamic() except it applies the test to a ListModel called
    // from a WorkerScript.

    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("model.qml")));
    QQuickItem *item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item != 0);

    QSignalSpy spyCount(&model, SIGNAL(countChanged()));

    if (script[0] == QLatin1Char('{') && script[script.length()-1] == QLatin1Char('}'))
        script = script.mid(1, script.length() - 2);
    QVariantList operations;
    foreach (const QString &s, script.split(';')) {
        if (!s.isEmpty())
            operations << s;
    }

    if (isValidErrorMessage(warning, dynamicRoles))
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());

    QVERIFY(QMetaObject::invokeMethod(item, "evalExpressionViaWorker",
            Q_ARG(QVariant, operations)));
    waitForWorker(item);
    QCOMPARE(QDeclarativeProperty(item, "result").read().toInt(), result);

    if (model.count() > 0)
        QVERIFY(spyCount.count() > 0);

    delete item;
    qApp->processEvents();
}

void tst_qdeclarativelistmodel::dynamic_worker_sync_data()
{
    dynamic_data();
}

void tst_qdeclarativelistmodel::dynamic_worker_sync()
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

    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("model.qml")));
    QQuickItem *item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item != 0);

    if (script[0] == QLatin1Char('{') && script[script.length()-1] == QLatin1Char('}'))
        script = script.mid(1, script.length() - 2);
    QVariantList operations;
    foreach (const QString &s, script.split(';')) {
        if (!s.isEmpty())
            operations << s;
    }

    if (isValidErrorMessage(warning, dynamicRoles))
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());

    // execute a set of commands on the worker list model, then check the
    // changes are reflected in the list model in the main thread
    QVERIFY(QMetaObject::invokeMethod(item, "evalExpressionViaWorker",
            Q_ARG(QVariant, operations.mid(0, operations.length()-1))));
    waitForWorker(item);

    QDeclarativeExpression e(eng.rootContext(), &model, operations.last().toString());
    QCOMPARE(e.evaluate().toInt(), result);

    delete item;
    qApp->processEvents();
}

void tst_qdeclarativelistmodel::enumerate()
{
    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("enumerate.qml")));
    QVERIFY(!component.isError());
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item != 0);

    QLatin1String expectedStrings[] = {
        QLatin1String("val1=1Y"),
        QLatin1String("val2=2Y"),
        QLatin1String("val3=strY"),
        QLatin1String("val4=falseN"),
        QLatin1String("val5=trueY")
    };

    int expectedStringCount = sizeof(expectedStrings) / sizeof(expectedStrings[0]);

    QStringList r = item->property("result").toString().split(":");

    int matchCount = 0;
    for (int i=0 ; i < expectedStringCount ; ++i) {
        const QLatin1String &expectedString = expectedStrings[i];

        QStringList::const_iterator it = r.begin();
        QStringList::const_iterator end = r.end();

        while (it != end) {
            if (it->compare(expectedString) == 0) {
                ++matchCount;
                break;
            }
            ++it;
        }
    }

    QVERIFY(matchCount == expectedStringCount);

    delete item;
}

void tst_qdeclarativelistmodel::error_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("error");

    QTest::newRow("id not allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { id: fred } }"
        << "ListElement: cannot use reserved \"id\" property";

    QTest::newRow("id allowed in ListModel")
        << "import QtQuick 2.0\nListModel { id:model }"
        << "";

    QTest::newRow("random properties not allowed in ListModel")
        << "import QtQuick 2.0\nListModel { foo:123 }"
        << "ListModel: undefined property 'foo'";

    QTest::newRow("random properties allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { foo:123 } }"
        << "";

    QTest::newRow("bindings not allowed in ListElement")
        << "import QtQuick 2.0\nRectangle { id: rect; ListModel { ListElement { foo: rect.color } } }"
        << "ListElement: cannot use script for property value";

    QTest::newRow("random object list properties allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { foo: [ ListElement { bar: 123 } ] } }"
        << "";

    QTest::newRow("default properties not allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { Item { } } }"
        << "ListElement: cannot contain nested elements";

    QTest::newRow("QML elements not allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { a: Item { } } }"
        << "ListElement: cannot contain nested elements";

    QTest::newRow("qualified ListElement supported")
        << "import QtQuick 2.0 as Foo\nFoo.ListModel { Foo.ListElement { a: 123 } }"
        << "";

    QTest::newRow("qualified ListElement required")
        << "import QtQuick 2.0 as Foo\nFoo.ListModel { ListElement { a: 123 } }"
        << "ListElement is not a type";

    QTest::newRow("unknown qualified ListElement not allowed")
        << "import QtQuick 2.0\nListModel { Foo.ListElement { a: 123 } }"
        << "Foo.ListElement - Foo is not a namespace";
}

void tst_qdeclarativelistmodel::error()
{
    QFETCH(QString, qml);
    QFETCH(QString, error);

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));
    if (error.isEmpty()) {
        QVERIFY(!component.isError());
    } else {
        QVERIFY(component.isError());
        QList<QDeclarativeError> errors = component.errors();
        QCOMPARE(errors.count(),1);
        QCOMPARE(errors.at(0).description(),error);
    }
}

void tst_qdeclarativelistmodel::syncError()
{
    QString qml = "import QtQuick 2.0\nListModel { id: lm; Component.onCompleted: lm.sync() }";
    QString error = "file:dummy.qml:2:1: QML ListModel: List sync() can only be called from a WorkerScript";

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));
    QTest::ignoreMessage(QtWarningMsg,error.toUtf8());
    QObject *obj = component.create();
    QVERIFY(obj);
    delete obj;
}

/*
    Test model changes from set() are available to the view
*/
void tst_qdeclarativelistmodel::set_data()
{
    QTest::addColumn<bool>("dynamicRoles");

    QTest::newRow("staticRoles") << false;
    QTest::newRow("dynamicRoles") << true;
}

void tst_qdeclarativelistmodel::set()
{
    QFETCH(bool, dynamicRoles);

    QDeclarativeEngine engine;
    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine::setContextForObject(&model,engine.rootContext());
    engine.rootContext()->setContextProperty("model", &model);

    RUNEXPR("model.append({test:false})");
    RUNEXPR("model.set(0, {test:true})");

    QCOMPARE(RUNEXPR("model.get(0).test").toBool(), true); // triggers creation of model cache
    QCOMPARE(model.data(0, model.roles()[0]), qVariantFromValue(true));

    RUNEXPR("model.set(0, {test:false})");
    QCOMPARE(RUNEXPR("model.get(0).test").toBool(), false); // tests model cache is updated
    QCOMPARE(model.data(0, model.roles()[0]), qVariantFromValue(false));

    QString warning = QString::fromLatin1("<Unknown File>: Can't create role for unsupported data type");
    if (isValidErrorMessage(warning, dynamicRoles))
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());
    QVariant invalidData = QColor();
    model.setProperty(0, "test", invalidData);
}

/*
    Test model changes on values returned by get() are available to the view
*/
void tst_qdeclarativelistmodel::get()
{
    QFETCH(QString, expression);
    QFETCH(int, index);
    QFETCH(QString, roleName);
    QFETCH(QVariant, roleValue);
    QFETCH(bool, dynamicRoles);

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(
        "import QtQuick 2.0\n"
        "ListModel {}\n", QUrl());
    QDeclarativeListModel *model = qobject_cast<QDeclarativeListModel*>(component.create());
    model->setDynamicRoles(dynamicRoles);
    engine.rootContext()->setContextProperty("model", model);

    RUNEXPR("model.append({roleA: 100})");
    RUNEXPR("model.append({roleA: 200, roleB: 400})");
    RUNEXPR("model.append({roleA: 200, roleB: 400})");
    RUNEXPR("model.append({roleC: {} })");
    RUNEXPR("model.append({roleD: [ { a:1, b:2 }, { c: 3 } ] })");

    QSignalSpy spy(model, SIGNAL(itemsChanged(int, int, QList<int>)));
    QDeclarativeExpression expr(engine.rootContext(), model, expression);
    expr.evaluate();
    QVERIFY(!expr.hasError());

    int role = roleFromName(model, roleName);
    QVERIFY(role >= 0);

    if (roleValue.type() == QVariant::List) {
        const QVariantList &list = roleValue.toList();
        QVERIFY(compareVariantList(list, model->data(index, role)));
    } else {
        QCOMPARE(model->data(index, role), roleValue);
    }

    QCOMPARE(spy.count(), 1);

    QList<QVariant> spyResult = spy.takeFirst();
    QCOMPARE(spyResult.at(0).toInt(), index);
    QCOMPARE(spyResult.at(1).toInt(), 1);  // only 1 item is modified at a time
    QCOMPARE(spyResult.at(2).value<QList<int> >(), (QList<int>() << role));

    delete model;
}

void tst_qdeclarativelistmodel::get_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("roleName");
    QTest::addColumn<QVariant>("roleValue");
    QTest::addColumn<bool>("dynamicRoles");

    for (int i=0 ; i < 2 ; ++i) {
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

void tst_qdeclarativelistmodel::get_worker()
{
    QFETCH(QString, expression);
    QFETCH(int, index);
    QFETCH(QString, roleName);
    QFETCH(QVariant, roleValue);
    QFETCH(bool, dynamicRoles);

    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("model.qml")));
    QQuickItem *item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item != 0);

    // Add some values like get() test
    RUNEVAL(item, "model.append({roleA: 100})");
    RUNEVAL(item, "model.append({roleA: 200, roleB: 400})");
    RUNEVAL(item, "model.append({roleA: 200, roleB: 400})");
    RUNEVAL(item, "model.append({roleC: {} })");
    RUNEVAL(item, "model.append({roleD: [ { a:1, b:2 }, { c: 3 } ] })");

    int role = roleFromName(&model, roleName);
    QVERIFY(role >= 0);

    QSignalSpy spy(&model, SIGNAL(itemsChanged(int, int, QList<int>)));

    // in the worker thread, change the model data and call sync()
    QVERIFY(QMetaObject::invokeMethod(item, "evalExpressionViaWorker",
            Q_ARG(QVariant, QStringList(expression))));
    waitForWorker(item);

    // see if we receive the model changes in the main thread's model
    if (roleValue.type() == QVariant::List) {
        const QVariantList &list = roleValue.toList();
        QVERIFY(compareVariantList(list, model.data(index, role)));
    } else {
        QCOMPARE(model.data(index, role), roleValue);
    }

    QCOMPARE(spy.count(), 1);

    QList<QVariant> spyResult = spy.takeFirst();
    QCOMPARE(spyResult.at(0).toInt(), index);
    QCOMPARE(spyResult.at(1).toInt(), 1);  // only 1 item is modified at a time
    QVERIFY(spyResult.at(2).value<QList<int> >().contains(role));
}

void tst_qdeclarativelistmodel::get_worker_data()
{
    get_data();
}

/*
    Test that the tests run in get() also work for nested list data
*/
void tst_qdeclarativelistmodel::get_nested()
{
    QFETCH(QString, expression);
    QFETCH(int, index);
    QFETCH(QString, roleName);
    QFETCH(QVariant, roleValue);
    QFETCH(bool, dynamicRoles);

    if (roleValue.type() == QVariant::Map)
        return;

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(
        "import QtQuick 2.0\n"
        "ListModel {}", QUrl());
    QDeclarativeListModel *model = qobject_cast<QDeclarativeListModel*>(component.create());
    model->setDynamicRoles(dynamicRoles);
    QVERIFY(component.errorString().isEmpty());
    QDeclarativeListModel *childModel;
    engine.rootContext()->setContextProperty("model", model);

    RUNEXPR("model.append({ listRoleA: [\n"
                            "{ roleA: 100 },\n"
                            "{ roleA: 200, roleB: 400 },\n"
                            "{ roleA: 200, roleB: 400 }, \n"
                            "{ roleC: {} }, \n"
                            "{ roleD: [ { a: 1, b:2 }, { c: 3 } ] } \n"
                            "] })\n");

    RUNEXPR("model.append({ listRoleA: [\n"
                            "{ roleA: 100 },\n"
                            "{ roleA: 200, roleB: 400 },\n"
                            "{ roleA: 200, roleB: 400 }, \n"
                            "{ roleC: {} }, \n"
                            "{ roleD: [ { a: 1, b:2 }, { c: 3 } ] } \n"
                            "],\n"
                            "listRoleB: [\n"
                            "{ roleA: 100 },\n"
                            "{ roleA: 200, roleB: 400 },\n"
                            "{ roleA: 200, roleB: 400 }, \n"
                            "{ roleC: {} }, \n"
                            "{ roleD: [ { a: 1, b:2 }, { c: 3 } ] } \n"
                            "],\n"
                            "listRoleC: [\n"
                            "{ roleA: 100 },\n"
                            "{ roleA: 200, roleB: 400 },\n"
                            "{ roleA: 200, roleB: 400 }, \n"
                            "{ roleC: {} }, \n"
                            "{ roleD: [ { a: 1, b:2 }, { c: 3 } ] } \n"
                            "] })\n");

    // Test setting the inner list data for:
    //  get(0).listRoleA
    //  get(1).listRoleA
    //  get(1).listRoleB
    //  get(1).listRoleC

    QList<QPair<int, QString> > testData;
    testData << qMakePair(0, QString("listRoleA"));
    testData << qMakePair(1, QString("listRoleA"));
    testData << qMakePair(1, QString("listRoleB"));
    testData << qMakePair(1, QString("listRoleC"));

    for (int i=0; i<testData.count(); i++) {
        int outerListIndex = testData[i].first;
        QString outerListRoleName = testData[i].second;
        int outerListRole = roleFromName(model, outerListRoleName);
        QVERIFY(outerListRole >= 0);

        childModel = qobject_cast<QDeclarativeListModel*>(model->data(outerListIndex, outerListRole).value<QObject*>());
        QVERIFY(childModel);

        QString extendedExpression = QString("get(%1).%2.%3").arg(outerListIndex).arg(outerListRoleName).arg(expression);
        QDeclarativeExpression expr(engine.rootContext(), model, extendedExpression);

        QSignalSpy spy(childModel, SIGNAL(itemsChanged(int, int, QList<int>)));
        expr.evaluate();
        QVERIFY(!expr.hasError());

        int role = roleFromName(childModel, roleName);
        QVERIFY(role >= 0);
        if (roleValue.type() == QVariant::List) {
            QVERIFY(compareVariantList(roleValue.toList(), childModel->data(index, role)));
        } else {
            QCOMPARE(childModel->data(index, role), roleValue);
        }
        QCOMPARE(spy.count(), 1);

        QList<QVariant> spyResult = spy.takeFirst();
        QCOMPARE(spyResult.at(0).toInt(), index);
        QCOMPARE(spyResult.at(1).toInt(), 1);  // only 1 item is modified at a time
        QCOMPARE(spyResult.at(2).value<QList<int> >(), (QList<int>() << role));
    }

    delete model;
}

void tst_qdeclarativelistmodel::get_nested_data()
{
    get_data();
}

//QTBUG-13754
void tst_qdeclarativelistmodel::crash_model_with_multiple_roles()
{
    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("multipleroles.qml")));
    QObject *rootItem = component.create();
    QVERIFY(component.errorString().isEmpty());
    QVERIFY(rootItem != 0);
    QDeclarativeListModel *model = rootItem->findChild<QDeclarativeListModel*>("listModel");
    QVERIFY(model != 0);

    // used to cause a crash in QDeclarativeVisualDataModel
    model->setProperty(0, "black", true);

    delete rootItem;
}

//QTBUG-15190
void tst_qdeclarativelistmodel::set_model_cache()
{
    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("setmodelcachelist.qml")));
    QObject *model = component.create();
    QVERIFY2(component.errorString().isEmpty(), QTest::toString(component.errorString()));
    QVERIFY(model != 0);
    QVERIFY(model->property("ok").toBool());

    delete model;
}

void tst_qdeclarativelistmodel::property_changes()
{
    QFETCH(QString, script_setup);
    QFETCH(QString, script_change);
    QFETCH(QString, roleName);
    QFETCH(int, listIndex);
    QFETCH(bool, itemsChanged);
    QFETCH(QString, testExpression);
    QFETCH(bool, dynamicRoles);

    QDeclarativeEngine engine;
    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine::setContextForObject(&model, engine.rootContext());
    engine.rootContext()->setContextObject(&model);

    QDeclarativeExpression expr(engine.rootContext(), &model, script_setup);
    expr.evaluate();
    QVERIFY2(!expr.hasError(), QTest::toString(expr.error().toString()));

    QString signalHandler = "on" + QString(roleName[0].toUpper()) + roleName.mid(1, roleName.length()) + "Changed:";
    QString qml = "import QtQuick 2.0\n"
                  "Connections {\n"
                        "property bool gotSignal: false\n"
                        "target: model.get(" + QString::number(listIndex) + ")\n"
                        + signalHandler + " gotSignal = true\n"
                  "}\n";

    QDeclarativeComponent component(&engine);
    component.setData(qml.toUtf8(), QUrl::fromLocalFile(""));
    engine.rootContext()->setContextProperty("model", &model);
    QObject *connectionsObject = component.create();
    QVERIFY2(component.errorString().isEmpty(), QTest::toString(component.errorString()));

    QSignalSpy spyItemsChanged(&model, SIGNAL(itemsChanged(int, int, QList<int>)));

    expr.setExpression(script_change);
    expr.evaluate();
    QVERIFY2(!expr.hasError(), QTest::toString(expr.error()));

    // test the object returned by get() emits the correct signals
    QCOMPARE(connectionsObject->property("gotSignal").toBool(), itemsChanged);

    // test itemsChanged() is emitted correctly
    if (itemsChanged) {
        QCOMPARE(spyItemsChanged.count(), 1);
        QCOMPARE(spyItemsChanged.at(0).at(0).toInt(), listIndex);
        QCOMPARE(spyItemsChanged.at(0).at(1).toInt(), 1);
    } else {
        QCOMPARE(spyItemsChanged.count(), 0);
    }

    expr.setExpression(testExpression);
    QCOMPARE(expr.evaluate().toBool(), true);

    delete connectionsObject;
}

void tst_qdeclarativelistmodel::property_changes_data()
{
    QTest::addColumn<QString>("script_setup");
    QTest::addColumn<QString>("script_change");
    QTest::addColumn<QString>("roleName");
    QTest::addColumn<int>("listIndex");
    QTest::addColumn<bool>("itemsChanged");
    QTest::addColumn<QString>("testExpression");
    QTest::addColumn<bool>("dynamicRoles");

    for (int i=0 ; i < 2 ; ++i) {
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
                << "b" << 0 << true << "get(0).b.count == 0" << dr;
    }
}

void tst_qdeclarativelistmodel::property_changes_worker()
{
    QFETCH(QString, script_setup);
    QFETCH(QString, script_change);
    QFETCH(QString, roleName);
    QFETCH(int, listIndex);
    QFETCH(bool, itemsChanged);
    QFETCH(bool, dynamicRoles);

    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("model.qml")));
    QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8());
    QQuickItem *item = createWorkerTest(&engine, &component, &model);
    QVERIFY(item != 0);

    QDeclarativeExpression expr(engine.rootContext(), &model, script_setup);
    expr.evaluate();
    QVERIFY2(!expr.hasError(), QTest::toString(expr.error().toString()));

    QSignalSpy spyItemsChanged(&model, SIGNAL(itemsChanged(int, int, QList<int>)));

    QVERIFY(QMetaObject::invokeMethod(item, "evalExpressionViaWorker",
            Q_ARG(QVariant, QStringList(script_change))));
    waitForWorker(item);

    // test itemsChanged() is emitted correctly
    if (itemsChanged) {
        QCOMPARE(spyItemsChanged.count(), 1);
        QCOMPARE(spyItemsChanged.at(0).at(0).toInt(), listIndex);
        QCOMPARE(spyItemsChanged.at(0).at(1).toInt(), 1);
    } else {
        QCOMPARE(spyItemsChanged.count(), 0);
    }

    delete item;
    qApp->processEvents();
}

void tst_qdeclarativelistmodel::property_changes_worker_data()
{
    property_changes_data();
}

void tst_qdeclarativelistmodel::clear_data()
{
    QTest::addColumn<bool>("dynamicRoles");

    QTest::newRow("staticRoles") << false;
    QTest::newRow("dynamicRoles") << true;
}

void tst_qdeclarativelistmodel::clear()
{
    QFETCH(bool, dynamicRoles);

    QDeclarativeEngine engine;
    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine::setContextForObject(&model, engine.rootContext());
    engine.rootContext()->setContextProperty("model", &model);

    model.clear();
    QCOMPARE(model.count(), 0);

    RUNEXPR("model.append({propertyA: \"value a\", propertyB: \"value b\"})");
    QCOMPARE(model.count(), 1);

    model.clear();
    QCOMPARE(model.count(), 0);

    RUNEXPR("model.append({propertyA: \"value a\", propertyB: \"value b\"})");
    RUNEXPR("model.append({propertyA: \"value a\", propertyB: \"value b\"})");
    QCOMPARE(model.count(), 2);

    model.clear();
    QCOMPARE(model.count(), 0);

    // clearing does not remove the roles
    RUNEXPR("model.append({propertyA: \"value a\", propertyB: \"value b\", propertyC: \"value c\"})");
    QList<int> roles = model.roles();
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.roles(), roles);
    QCOMPARE(model.toString(roles[0]), QString("propertyA"));
    QCOMPARE(model.toString(roles[1]), QString("propertyB"));
    QCOMPARE(model.toString(roles[2]), QString("propertyC"));
}

void tst_qdeclarativelistmodel::signal_handlers_data()
{
    QTest::addColumn<bool>("dynamicRoles");

    QTest::newRow("staticRoles") << false;
    QTest::newRow("dynamicRoles") << true;
}

void tst_qdeclarativelistmodel::signal_handlers()
{
    QFETCH(bool, dynamicRoles);

    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("signalhandlers.qml")));
    QObject *model = component.create();
    QDeclarativeListModel *lm = qobject_cast<QDeclarativeListModel *>(model);
    QVERIFY(lm != 0);
    lm->setDynamicRoles(dynamicRoles);
    QVERIFY2(component.errorString().isEmpty(), QTest::toString(component.errorString()));
    QVERIFY(model != 0);
    QVERIFY(model->property("ok").toBool());

    delete model;
}

void tst_qdeclarativelistmodel::worker_sync_data()
{
    QTest::addColumn<bool>("dynamicRoles");

    QTest::newRow("staticRoles") << false;
    QTest::newRow("dynamicRoles") << true;
}

void tst_qdeclarativelistmodel::worker_sync()
{
    QFETCH(bool, dynamicRoles);

    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("workersync.qml")));
    QQuickItem *item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item != 0);

    QVERIFY(model.count() == 0);

    QVERIFY(QMetaObject::invokeMethod(item, "addItem0"));

    QVERIFY(model.count() == 2);
    QVariant childData = model.data(0, 0);
    QDeclarativeListModel *childModel = qobject_cast<QDeclarativeListModel *>(childData.value<QObject *>());
    QVERIFY(childModel);
    QVERIFY(childModel->count() == 1);

    QSignalSpy spyModelInserted(&model, SIGNAL(itemsInserted(int,int)));
    QSignalSpy spyChildInserted(childModel, SIGNAL(itemsInserted(int,int)));

    QVERIFY(QMetaObject::invokeMethod(item, "addItemViaWorker"));
    waitForWorker(item);

    QVERIFY(model.count() == 2);
    QVERIFY(childModel->count() == 1);
    QVERIFY(spyModelInserted.count() == 0);
    QVERIFY(spyChildInserted.count() == 0);

    QVERIFY(QMetaObject::invokeMethod(item, "doSync"));
    waitForWorker(item);

    QVERIFY(model.count() == 2);
    QVERIFY(childModel->count() == 2);
    QVERIFY(spyModelInserted.count() == 0);
    QVERIFY(spyChildInserted.count() == 1);

    QVERIFY(QMetaObject::invokeMethod(item, "addItemViaWorker"));
    waitForWorker(item);

    QVERIFY(model.count() == 2);
    QVERIFY(childModel->count() == 2);
    QVERIFY(spyModelInserted.count() == 0);
    QVERIFY(spyChildInserted.count() == 1);

    QVERIFY(QMetaObject::invokeMethod(item, "doSync"));
    waitForWorker(item);

    QVERIFY(model.count() == 2);
    QVERIFY(childModel->count() == 3);
    QVERIFY(spyModelInserted.count() == 0);
    QVERIFY(spyChildInserted.count() == 2);

    delete item;
    qApp->processEvents();
}

void tst_qdeclarativelistmodel::worker_remove_element_data()
{
    worker_sync_data();
}

void tst_qdeclarativelistmodel::worker_remove_element()
{
    QFETCH(bool, dynamicRoles);

    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("workerremoveelement.qml")));
    QQuickItem *item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item != 0);

    QSignalSpy spyModelRemoved(&model, SIGNAL(itemsRemoved(int,int)));

    QVERIFY(model.count() == 0);
    QVERIFY(spyModelRemoved.count() == 0);

    QVERIFY(QMetaObject::invokeMethod(item, "addItem"));

    QVERIFY(model.count() == 1);

    QVERIFY(QMetaObject::invokeMethod(item, "removeItemViaWorker"));
    waitForWorker(item);

    QVERIFY(model.count() == 1);
    QVERIFY(spyModelRemoved.count() == 0);

    QVERIFY(QMetaObject::invokeMethod(item, "doSync"));
    waitForWorker(item);

    QVERIFY(model.count() == 0);
    QVERIFY(spyModelRemoved.count() == 1);

    delete item;
    qApp->processEvents();
}

void tst_qdeclarativelistmodel::worker_remove_list_data()
{
    worker_sync_data();
}

void tst_qdeclarativelistmodel::worker_remove_list()
{
    QFETCH(bool, dynamicRoles);

    QDeclarativeListModel model;
    model.setDynamicRoles(dynamicRoles);
    QDeclarativeEngine eng;
    QDeclarativeComponent component(&eng, QUrl::fromLocalFile(TESTDATA("workerremovelist.qml")));
    QQuickItem *item = createWorkerTest(&eng, &component, &model);
    QVERIFY(item != 0);

    QSignalSpy spyModelRemoved(&model, SIGNAL(itemsRemoved(int,int)));

    QVERIFY(model.count() == 0);
    QVERIFY(spyModelRemoved.count() == 0);

    QVERIFY(QMetaObject::invokeMethod(item, "addList"));

    QVERIFY(model.count() == 1);

    QVERIFY(QMetaObject::invokeMethod(item, "removeListViaWorker"));
    waitForWorker(item);

    QVERIFY(model.count() == 1);
    QVERIFY(spyModelRemoved.count() == 0);

    QVERIFY(QMetaObject::invokeMethod(item, "doSync"));
    waitForWorker(item);

    QVERIFY(model.count() == 0);
    QVERIFY(spyModelRemoved.count() == 1);

    delete item;
    qApp->processEvents();
}

void tst_qdeclarativelistmodel::role_mode_data()
{
    QTest::addColumn<QString>("script");
    QTest::addColumn<int>("result");
    QTest::addColumn<QString>("warning");

    QTest::newRow("default0") << "{dynamicRoles}" << 0 << "";
    QTest::newRow("default1") << "{append({'a':1});dynamicRoles}" << 0 << "";

    QTest::newRow("enableDynamic0") << "{dynamicRoles=true;dynamicRoles}" << 1 << "";
    QTest::newRow("enableDynamic1") << "{append({'a':1});dynamicRoles=true;dynamicRoles}" << 0 << "<Unknown File>: QML ListModel: unable to enable dynamic roles as this model is not empty!";
    QTest::newRow("enableDynamic2") << "{dynamicRoles=true;append({'a':1});dynamicRoles=false;dynamicRoles}" << 1 << "<Unknown File>: QML ListModel: unable to enable static roles as this model is not empty!";
}

void tst_qdeclarativelistmodel::role_mode()
{
    QFETCH(QString, script);
    QFETCH(int, result);
    QFETCH(QString, warning);

    QDeclarativeEngine engine;
    QDeclarativeListModel model;
    QDeclarativeEngine::setContextForObject(&model,engine.rootContext());
    engine.rootContext()->setContextObject(&model);
    QDeclarativeExpression e(engine.rootContext(), &model, script);
    if (!warning.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());

    int actual = e.evaluate().toInt();
    if (e.hasError())
        qDebug() << e.error(); // errors not expected

    QCOMPARE(actual,result);
}

void tst_qdeclarativelistmodel::dynamic_role_data()
{
    QTest::addColumn<QString>("preamble");
    QTest::addColumn<QString>("script");
    QTest::addColumn<int>("result");

    QTest::newRow("sync1") << "{append({'a':[{'b':1},{'b':2}]})}" << "{get(0).a = 'string';count}" << 1;
}

void tst_qdeclarativelistmodel::dynamic_role()
{
    QFETCH(QString, preamble);
    QFETCH(QString, script);
    QFETCH(int, result);

    QDeclarativeListModel model;
    model.setDynamicRoles(true);
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("model.qml")));
    QQuickItem *item = createWorkerTest(&engine, &component, &model);
    QVERIFY(item != 0);

    QDeclarativeExpression preExp(engine.rootContext(), &model, preamble);
    QCOMPARE(preExp.evaluate().toInt(), 0);

    if (script[0] == QLatin1Char('{') && script[script.length()-1] == QLatin1Char('}'))
        script = script.mid(1, script.length() - 2);
    QVariantList operations;
    foreach (const QString &s, script.split(';')) {
        if (!s.isEmpty())
            operations << s;
    }

    // execute a set of commands on the worker list model, then check the
    // changes are reflected in the list model in the main thread
    QVERIFY(QMetaObject::invokeMethod(item, "evalExpressionViaWorker",
            Q_ARG(QVariant, operations.mid(0, operations.length()-1))));
    waitForWorker(item);

    QDeclarativeExpression e(engine.rootContext(), &model, operations.last().toString());
    QCOMPARE(e.evaluate().toInt(), result);

    delete item;
    qApp->processEvents();
}

QTEST_MAIN(tst_qdeclarativelistmodel)

#include "tst_qdeclarativelistmodel.moc"
