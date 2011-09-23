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
#include "../../../shared/util.h"
#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QStandardItemModel>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qsgview.h>
#include <private/qsglistview_p.h>
#include <private/qsgtext_p.h>
#include <private/qsgvisualdatamodel_p.h>
#include <private/qdeclarativevaluetype_p.h>
#include <private/qdeclarativechangeset_p.h>
#include <private/qdeclarativeengine_p.h>
#include <math.h>
#include <QtOpenGL/QGLShaderProgram>

template <typename T, int N> int lengthOf(const T (&)[N]) { return N; }

static void initStandardTreeModel(QStandardItemModel *model)
{
    QStandardItem *item;
    item = new QStandardItem(QLatin1String("Row 1 Item"));
    model->insertRow(0, item);

    item = new QStandardItem(QLatin1String("Row 2 Item"));
    item->setCheckable(true);
    model->insertRow(1, item);

    QStandardItem *childItem = new QStandardItem(QLatin1String("Row 2 Child Item"));
    item->setChild(0, childItem);

    item = new QStandardItem(QLatin1String("Row 3 Item"));
    item->setIcon(QIcon());
    model->insertRow(2, item);
}

class SingleRoleModel : public QAbstractListModel
{
    Q_OBJECT

public:
    SingleRoleModel(const QByteArray &role = "name", QObject *parent = 0) {
        QHash<int, QByteArray> roles;
        roles.insert(Qt::DisplayRole , role);
        setRoleNames(roles);
        list << "one" << "two" << "three" << "four";
    }

    void emitMove(int sourceFirst, int sourceLast, int destinationChild) {
        emit beginMoveRows(QModelIndex(), sourceFirst, sourceLast, QModelIndex(), destinationChild);
        emit endMoveRows();
    }

    QStringList list;

public slots:
    void set(int idx, QString string) {
        list[idx] = string;
        emit dataChanged(index(idx,0), index(idx,0));
    }

protected:
    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        return list.count();
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
        if (role == Qt::DisplayRole)
            return list.at(index.row());
        return QVariant();
    }
};


class tst_qsgvisualdatamodel : public QObject
{
    Q_OBJECT
public:
    tst_qsgvisualdatamodel();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void rootIndex();
    void updateLayout();
    void childChanged();
    void objectListModel();
    void singleRole();
    void modelProperties();
    void noDelegate();
    void qaimRowsMoved();
    void qaimRowsMoved_data();
    void remove();
    void move();
    void groups();
    void get();

private:
    template <int N> void groups_verify(
            const SingleRoleModel &model,
            QSGItem *contentItem,
            const int (&mIndex)[N],
            const int (&iIndex)[N],
            const int (&vIndex)[N],
            const int (&sIndex)[N],
            const bool (&vMember)[N],
            const bool (&sMember)[N]);

    template <int N> void get_verify(
            const SingleRoleModel &model,
            QSGVisualDataModel *visualModel,
            QSGVisualDataGroup *visibleItems,
            QSGVisualDataGroup *selectedItems,
            const int (&mIndex)[N],
            const int (&iIndex)[N],
            const int (&vIndex)[N],
            const int (&sIndex)[N],
            const bool (&vMember)[N],
            const bool (&sMember)[N]);

    bool failed;
    QDeclarativeEngine engine;
    template<typename T>
    T *findItem(QSGItem *parent, const QString &objectName, int index);
};

Q_DECLARE_METATYPE(QDeclarativeChangeSet)

void tst_qsgvisualdatamodel::initTestCase()
{
    qRegisterMetaType<QDeclarativeChangeSet>();
}

void tst_qsgvisualdatamodel::cleanupTestCase()
{

}
class DataObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)

public:
    DataObject(QObject *parent=0) : QObject(parent) {}
    DataObject(const QString &name, const QString &color, QObject *parent=0)
        : QObject(parent), m_name(name), m_color(color) { }


    QString name() const { return m_name; }
    void setName(const QString &name) {
        if (name != m_name) {
            m_name = name;
            emit nameChanged();
        }
    }

    QString color() const { return m_color; }
    void setColor(const QString &color) {
        if (color != m_color) {
            m_color = color;
            emit colorChanged();
        }
    }

signals:
    void nameChanged();
    void colorChanged();

private:
    QString m_name;
    QString m_color;
};

template <typename T> static T evaluate(QObject *scope, const QString &expression)
{
    QDeclarativeExpression expr(qmlContext(scope), scope, expression);
    T result = expr.evaluate().value<T>();
    if (expr.hasError())
        qWarning() << expr.error().toString();
    return result;
}

template <> void evaluate<void>(QObject *scope, const QString &expression)
{
    QDeclarativeExpression expr(qmlContext(scope), scope, expression);
    expr.evaluate();
    if (expr.hasError())
        qWarning() << expr.error().toString();
}

tst_qsgvisualdatamodel::tst_qsgvisualdatamodel()
{
}

void tst_qsgvisualdatamodel::rootIndex()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/visualdatamodel.qml"));

    QStandardItemModel model;
    initStandardTreeModel(&model);

    engine.rootContext()->setContextProperty("myModel", &model);

    QSGVisualDataModel *obj = qobject_cast<QSGVisualDataModel*>(c.create());
    QVERIFY(obj != 0);

    QMetaObject::invokeMethod(obj, "setRoot");
    QVERIFY(qvariant_cast<QModelIndex>(obj->rootIndex()) == model.index(0,0));

    QMetaObject::invokeMethod(obj, "setRootToParent");
    QVERIFY(qvariant_cast<QModelIndex>(obj->rootIndex()) == QModelIndex());

    QMetaObject::invokeMethod(obj, "setRoot");
    QVERIFY(qvariant_cast<QModelIndex>(obj->rootIndex()) == model.index(0,0));
    model.clear(); // will emit modelReset()
    QVERIFY(qvariant_cast<QModelIndex>(obj->rootIndex()) == QModelIndex());

    delete obj;
}

void tst_qsgvisualdatamodel::updateLayout()
{
    QSGView view;

    QStandardItemModel model;
    initStandardTreeModel(&model);

    view.rootContext()->setContextProperty("myModel", &model);

    view.setSource(QUrl::fromLocalFile(SRCDIR "/data/datalist.qml"));

    QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
    QVERIFY(listview != 0);

    QSGItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    QSGText *name = findItem<QSGText>(contentItem, "display", 0);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 1 Item"));
    name = findItem<QSGText>(contentItem, "display", 1);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 2 Item"));
    name = findItem<QSGText>(contentItem, "display", 2);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 3 Item"));

    model.invisibleRootItem()->sortChildren(0, Qt::DescendingOrder);

    name = findItem<QSGText>(contentItem, "display", 0);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 3 Item"));
    name = findItem<QSGText>(contentItem, "display", 1);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 2 Item"));
    name = findItem<QSGText>(contentItem, "display", 2);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 1 Item"));
}

void tst_qsgvisualdatamodel::childChanged()
{
    QSGView view;

    QStandardItemModel model;
    initStandardTreeModel(&model);

    view.rootContext()->setContextProperty("myModel", &model);

    view.setSource(QUrl::fromLocalFile(SRCDIR "/data/datalist.qml"));

    QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
    QVERIFY(listview != 0);

    QSGItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    QSGVisualDataModel *vdm = listview->findChild<QSGVisualDataModel*>("visualModel");
    vdm->setRootIndex(QVariant::fromValue(model.indexFromItem(model.item(1,0))));
    QCOMPARE(listview->count(), 1);

    QSGText *name = findItem<QSGText>(contentItem, "display", 0);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 2 Child Item"));

    model.item(1,0)->child(0,0)->setText("Row 2 updated child");

    name = findItem<QSGText>(contentItem, "display", 0);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 2 updated child"));

    model.item(1,0)->appendRow(new QStandardItem(QLatin1String("Row 2 Child Item 2")));
    QCOMPARE(listview->count(), 2);

    name = findItem<QSGText>(contentItem, "display", 1);
    QVERIFY(name != 0);
    QCOMPARE(name->text(), QString("Row 2 Child Item 2"));

    model.item(1,0)->takeRow(1);
    name = findItem<QSGText>(contentItem, "display", 1);
    QVERIFY(name == 0);

    vdm->setRootIndex(QVariant::fromValue(QModelIndex()));
    QCOMPARE(listview->count(), 3);
    name = findItem<QSGText>(contentItem, "display", 0);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 1 Item"));
    name = findItem<QSGText>(contentItem, "display", 1);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 2 Item"));
    name = findItem<QSGText>(contentItem, "display", 2);
    QVERIFY(name);
    QCOMPARE(name->text(), QString("Row 3 Item"));
}

void tst_qsgvisualdatamodel::objectListModel()
{
    QSGView view;

    QList<QObject*> dataList;
    dataList.append(new DataObject("Item 1", "red"));
    dataList.append(new DataObject("Item 2", "green"));
    dataList.append(new DataObject("Item 3", "blue"));
    dataList.append(new DataObject("Item 4", "yellow"));

    QDeclarativeContext *ctxt = view.rootContext();
    ctxt->setContextProperty("myModel", QVariant::fromValue(dataList));

    view.setSource(QUrl::fromLocalFile(SRCDIR "/data/objectlist.qml"));

    QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
    QVERIFY(listview != 0);

    QSGItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    QSGText *name = findItem<QSGText>(contentItem, "name", 0);
    QCOMPARE(name->text(), QString("Item 1"));

    QSGText *section = findItem<QSGText>(contentItem, "section", 0);
    QCOMPARE(section->text(), QString("Item 1"));

    dataList[0]->setProperty("name", QLatin1String("Changed"));
    QCOMPARE(name->text(), QString("Changed"));
}

void tst_qsgvisualdatamodel::singleRole()
{
    {
        QSGView view;

        SingleRoleModel model;

        QDeclarativeContext *ctxt = view.rootContext();
        ctxt->setContextProperty("myModel", &model);

        view.setSource(QUrl::fromLocalFile(SRCDIR "/data/singlerole1.qml"));

        QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
        QVERIFY(listview != 0);

        QSGItem *contentItem = listview->contentItem();
        QVERIFY(contentItem != 0);

        QSGText *name = findItem<QSGText>(contentItem, "name", 1);
        QCOMPARE(name->text(), QString("two"));

        model.set(1, "Changed");
        QCOMPARE(name->text(), QString("Changed"));
    }
    {
        QSGView view;

        SingleRoleModel model;

        QDeclarativeContext *ctxt = view.rootContext();
        ctxt->setContextProperty("myModel", &model);

        view.setSource(QUrl::fromLocalFile(SRCDIR "/data/singlerole2.qml"));

        QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
        QVERIFY(listview != 0);

        QSGItem *contentItem = listview->contentItem();
        QVERIFY(contentItem != 0);

        QSGText *name = findItem<QSGText>(contentItem, "name", 1);
        QCOMPARE(name->text(), QString("two"));

        model.set(1, "Changed");
        QCOMPARE(name->text(), QString("Changed"));
    }
    {
        QSGView view;

        SingleRoleModel model("modelData");

        QDeclarativeContext *ctxt = view.rootContext();
        ctxt->setContextProperty("myModel", &model);

        view.setSource(QUrl::fromLocalFile(SRCDIR "/data/singlerole2.qml"));

        QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
        QVERIFY(listview != 0);

        QSGItem *contentItem = listview->contentItem();
        QVERIFY(contentItem != 0);

        QSGText *name = findItem<QSGText>(contentItem, "name", 1);
        QCOMPARE(name->text(), QString("two"));

        model.set(1, "Changed");
        QCOMPARE(name->text(), QString("Changed"));
    }
}

void tst_qsgvisualdatamodel::modelProperties()
{
    {
        QSGView view;

        SingleRoleModel model;

        QDeclarativeContext *ctxt = view.rootContext();
        ctxt->setContextProperty("myModel", &model);

        view.setSource(QUrl::fromLocalFile(SRCDIR "/data/modelproperties.qml"));

        QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
        QVERIFY(listview != 0);

        QSGItem *contentItem = listview->contentItem();
        QVERIFY(contentItem != 0);

        QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", 1);
        QVERIFY(delegate);
        QCOMPARE(delegate->property("test1").toString(),QString("two"));
        QCOMPARE(delegate->property("test2").toString(),QString("two"));
        QCOMPARE(delegate->property("test3").toString(),QString("two"));
        QCOMPARE(delegate->property("test4").toString(),QString("two"));
        QVERIFY(!delegate->property("test9").isValid());
        QCOMPARE(delegate->property("test5").toString(),QString(""));
        QVERIFY(delegate->property("test6").value<QObject*>() != 0);
        QCOMPARE(delegate->property("test7").toInt(),1);
        QCOMPARE(delegate->property("test8").toInt(),1);
    }

    {
        QSGView view;

        QList<QObject*> dataList;
        dataList.append(new DataObject("Item 1", "red"));
        dataList.append(new DataObject("Item 2", "green"));
        dataList.append(new DataObject("Item 3", "blue"));
        dataList.append(new DataObject("Item 4", "yellow"));

        QDeclarativeContext *ctxt = view.rootContext();
        ctxt->setContextProperty("myModel", QVariant::fromValue(dataList));

        view.setSource(QUrl::fromLocalFile(SRCDIR "/data/modelproperties.qml"));

        QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
        QVERIFY(listview != 0);

        QSGItem *contentItem = listview->contentItem();
        QVERIFY(contentItem != 0);

        QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", 1);
        QVERIFY(delegate);
        QCOMPARE(delegate->property("test1").toString(),QString("Item 2"));
        QCOMPARE(delegate->property("test2").toString(),QString("Item 2"));
        QVERIFY(qobject_cast<DataObject*>(delegate->property("test3").value<QObject*>()) != 0);
        QVERIFY(qobject_cast<DataObject*>(delegate->property("test4").value<QObject*>()) != 0);
        QCOMPARE(delegate->property("test5").toString(),QString("Item 2"));
        QCOMPARE(delegate->property("test9").toString(),QString("Item 2"));
        QVERIFY(delegate->property("test6").value<QObject*>() != 0);
        QCOMPARE(delegate->property("test7").toInt(),1);
        QCOMPARE(delegate->property("test8").toInt(),1);
    }

    {
        QSGView view;

        QStandardItemModel model;
        initStandardTreeModel(&model);

        view.rootContext()->setContextProperty("myModel", &model);

        QUrl source(QUrl::fromLocalFile(SRCDIR "/data/modelproperties2.qml"));

        //3 items, 3 warnings each
        QTest::ignoreMessage(QtWarningMsg, source.toString().toLatin1() + ":13: ReferenceError: Can't find variable: modelData");
        QTest::ignoreMessage(QtWarningMsg, source.toString().toLatin1() + ":13: ReferenceError: Can't find variable: modelData");
        QTest::ignoreMessage(QtWarningMsg, source.toString().toLatin1() + ":13: ReferenceError: Can't find variable: modelData");
        QTest::ignoreMessage(QtWarningMsg, source.toString().toLatin1() + ":11: ReferenceError: Can't find variable: modelData");
        QTest::ignoreMessage(QtWarningMsg, source.toString().toLatin1() + ":11: ReferenceError: Can't find variable: modelData");
        QTest::ignoreMessage(QtWarningMsg, source.toString().toLatin1() + ":11: ReferenceError: Can't find variable: modelData");
        QTest::ignoreMessage(QtWarningMsg, source.toString().toLatin1() + ":17: TypeError: Cannot read property 'display' of undefined");
        QTest::ignoreMessage(QtWarningMsg, source.toString().toLatin1() + ":17: TypeError: Cannot read property 'display' of undefined");
        QTest::ignoreMessage(QtWarningMsg, source.toString().toLatin1() + ":17: TypeError: Cannot read property 'display' of undefined");

        view.setSource(source);

        QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
        QVERIFY(listview != 0);

        QSGItem *contentItem = listview->contentItem();
        QVERIFY(contentItem != 0);

        QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", 1);
        QVERIFY(delegate);
        QCOMPARE(delegate->property("test1").toString(),QString("Row 2 Item"));
        QCOMPARE(delegate->property("test2").toString(),QString("Row 2 Item"));
        QVERIFY(!delegate->property("test3").isValid());
        QVERIFY(!delegate->property("test4").isValid());
        QVERIFY(!delegate->property("test5").isValid());
        QVERIFY(!delegate->property("test9").isValid());
        QVERIFY(delegate->property("test6").value<QObject*>() != 0);
        QCOMPARE(delegate->property("test7").toInt(),1);
        QCOMPARE(delegate->property("test8").toInt(),1);
    }

    //### should also test QStringList and QVariantList
}

void tst_qsgvisualdatamodel::noDelegate()
{
    QSGView view;

    QStandardItemModel model;
    initStandardTreeModel(&model);

    view.rootContext()->setContextProperty("myModel", &model);

    view.setSource(QUrl::fromLocalFile(SRCDIR "/data/datalist.qml"));

    QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
    QVERIFY(listview != 0);

    QSGVisualDataModel *vdm = listview->findChild<QSGVisualDataModel*>("visualModel");
    QVERIFY(vdm != 0);
    QCOMPARE(vdm->count(), 3);

    vdm->setDelegate(0);
    QCOMPARE(vdm->count(), 0);
}


void tst_qsgvisualdatamodel::qaimRowsMoved()
{
    // Test parameters passed in QAIM::rowsMoved() signal are converted correctly
    // when translated and emitted as the QListModelInterface::itemsMoved() signal
    QFETCH(int, sourceFirst);
    QFETCH(int, sourceLast);
    QFETCH(int, destinationChild);
    QFETCH(int, expectFrom);
    QFETCH(int, expectTo);
    QFETCH(int, expectCount);

    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/visualdatamodel.qml"));

    SingleRoleModel model;
    model.list.clear();
    for (int i=0; i<30; i++)
        model.list << ("item " + i);
    engine.rootContext()->setContextProperty("myModel", &model);

    QSGVisualDataModel *obj = qobject_cast<QSGVisualDataModel*>(c.create());
    QVERIFY(obj != 0);

    QSignalSpy spy(obj, SIGNAL(modelUpdated(QDeclarativeChangeSet,bool)));
    model.emitMove(sourceFirst, sourceLast, destinationChild);
    // QAbstractItemModel also emits the changed signal when items are moved.
    QCOMPARE(spy.count(), 2);

    bool move = false;
    for (int i = 0; i < 2; ++i) {
        QCOMPARE(spy[1].count(), 2);
        QDeclarativeChangeSet changeSet = spy[i][0].value<QDeclarativeChangeSet>();
        if (!changeSet.changes().isEmpty())
            continue;
        move = true;
        QCOMPARE(changeSet.removes().count(), 1);
        QCOMPARE(changeSet.removes().at(0).index, expectFrom);
        QCOMPARE(changeSet.removes().at(0).count, expectCount);
        QCOMPARE(changeSet.inserts().count(), 1);
        QCOMPARE(changeSet.inserts().at(0).index, expectTo);
        QCOMPARE(changeSet.inserts().at(0).count, expectCount);
        QCOMPARE(changeSet.removes().at(0).moveId, changeSet.inserts().at(0).moveId);
        QCOMPARE(spy[i][1].toBool(), false);
    }
    QVERIFY(move);

    delete obj;
}

void tst_qsgvisualdatamodel::qaimRowsMoved_data()
{
    QTest::addColumn<int>("sourceFirst");
    QTest::addColumn<int>("sourceLast");
    QTest::addColumn<int>("destinationChild");
    QTest::addColumn<int>("expectFrom");
    QTest::addColumn<int>("expectTo");
    QTest::addColumn<int>("expectCount");

    QTest::newRow("move 1 forward")
        << 1 << 1 << 6
        << 1 << 5 << 1;

    QTest::newRow("move 1 backwards")
        << 4 << 4 << 1
        << 4 << 1 << 1;

    QTest::newRow("move multiple forwards")
        << 0 << 2 << 13
        << 0 << 10 << 3;

    QTest::newRow("move multiple forwards, with same to")
        << 0 << 1 << 3
        << 0 << 1 << 2;

    QTest::newRow("move multiple backwards")
        << 10 << 14 << 1
        << 10 << 1 << 5;
}

void tst_qsgvisualdatamodel::remove()
{
    QSGView view;

    SingleRoleModel model;
    model.list = QStringList()
            << "one"
            << "two"
            << "three"
            << "four"
            << "five"
            << "six"
            << "seven"
            << "eight"
            << "nine"
            << "ten"
            << "eleven"
            << "twelve";

    QDeclarativeContext *ctxt = view.rootContext();
    ctxt->setContextProperty("myModel", &model);

    view.setSource(QUrl::fromLocalFile(SRCDIR "/data/groups.qml"));

    QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
    QVERIFY(listview != 0);

    QSGItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    QSGVisualDataModel *visualModel = qobject_cast<QSGVisualDataModel *>(qvariant_cast<QObject *>(listview->model()));
    QVERIFY(visualModel);

    {
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        static const int mIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int iIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };

        for (int i = 0; i < lengthOf(mIndex); ++i) {
            QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", mIndex[i]);
            QVERIFY(delegate);
            QCOMPARE(delegate->property("test1").toString(), model.list.at(mIndex[i]));
            QCOMPARE(delegate->property("test2").toInt(), mIndex[i]);
            QCOMPARE(delegate->property("test3").toInt(), iIndex[i]);
        }
    } {
        evaluate<void>(visualModel, "items.remove(2)");
        QCOMPARE(listview->count(), 11);
        QCOMPARE(visualModel->items()->count(), 11);
        static const int mIndex[] = { 0, 1, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int iIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10 };

        for (int i = 0; i < lengthOf(mIndex); ++i) {
            QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", mIndex[i]);
            QVERIFY(delegate);
            QCOMPARE(delegate->property("test1").toString(), model.list.at(mIndex[i]));
            QCOMPARE(delegate->property("test2").toInt(), mIndex[i]);
            QCOMPARE(delegate->property("test3").toInt(), iIndex[i]);
        }
    } {
        evaluate<void>(visualModel, "items.remove(1, 4)");
        QCOMPARE(listview->count(), 7);
        QCOMPARE(visualModel->items()->count(), 7);
        static const int mIndex[] = { 0, 6, 7, 8, 9,10,11 };
        static const int iIndex[] = { 0, 1, 2, 3, 4, 5, 6 };

        for (int i = 0; i < lengthOf(mIndex); ++i) {
            QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", mIndex[i]);
            QVERIFY(delegate);
            QCOMPARE(delegate->property("test1").toString(), model.list.at(mIndex[i]));
            QCOMPARE(delegate->property("test2").toInt(), mIndex[i]);
            QCOMPARE(delegate->property("test3").toInt(), iIndex[i]);
        }
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: remove: index out of range");
        evaluate<void>(visualModel, "items.remove(-8, 4)");
        QCOMPARE(listview->count(), 7);
        QCOMPARE(visualModel->items()->count(), 7);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: remove: index out of range");
        evaluate<void>(visualModel, "items.remove(12, 2)");
        QCOMPARE(listview->count(), 7);
        QCOMPARE(visualModel->items()->count(), 7);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: remove: index out of range");
        evaluate<void>(visualModel, "items.remove(5, 3)");
        QCOMPARE(listview->count(), 7);
        QCOMPARE(visualModel->items()->count(), 7);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: remove: invalid count");
        evaluate<void>(visualModel, "items.remove(5, -2)");
        QCOMPARE(listview->count(), 7);
        QCOMPARE(visualModel->items()->count(), 7);
    }
}

void tst_qsgvisualdatamodel::move()
{
    QSGView view;

    SingleRoleModel model;
    model.list = QStringList()
            << "one"
            << "two"
            << "three"
            << "four"
            << "five"
            << "six"
            << "seven"
            << "eight"
            << "nine"
            << "ten"
            << "eleven"
            << "twelve";

    QDeclarativeContext *ctxt = view.rootContext();
    ctxt->setContextProperty("myModel", &model);

    view.setSource(QUrl::fromLocalFile(SRCDIR "/data/groups.qml"));

    QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
    QVERIFY(listview != 0);

    QSGItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    QSGVisualDataModel *visualModel = qobject_cast<QSGVisualDataModel *>(qvariant_cast<QObject *>(listview->model()));
    QVERIFY(visualModel);

    {
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        static const int mIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int iIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };

        for (int i = 0; i < lengthOf(mIndex); ++i) {
            QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", mIndex[i]);
            QVERIFY(delegate);
            QCOMPARE(delegate->property("test1").toString(), model.list.at(mIndex[i]));
            QCOMPARE(delegate->property("test2").toInt(), mIndex[i]);
            QCOMPARE(delegate->property("test3").toInt(), iIndex[i]);
        }
    } {
        evaluate<void>(visualModel, "items.move(2, 4)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        static const int mIndex[] = { 0, 1, 3, 4, 2, 5, 6, 7, 8, 9,10,11 };
        static const int iIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };

        for (int i = 0; i < lengthOf(mIndex); ++i) {
            QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", mIndex[i]);
            QVERIFY(delegate);
            QCOMPARE(delegate->property("test1").toString(), model.list.at(mIndex[i]));
            QCOMPARE(delegate->property("test2").toInt(), mIndex[i]);
            QCOMPARE(delegate->property("test3").toInt(), iIndex[i]);
        }
    } {
        evaluate<void>(visualModel, "items.move(4, 2)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        static const int mIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int iIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };

        for (int i = 0; i < lengthOf(mIndex); ++i) {
            QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", mIndex[i]);
            QVERIFY(delegate);
            QCOMPARE(delegate->property("test1").toString(), model.list.at(mIndex[i]));
            QCOMPARE(delegate->property("test2").toInt(), mIndex[i]);
            QCOMPARE(delegate->property("test3").toInt(), iIndex[i]);
        }
    } {
        evaluate<void>(visualModel, "items.move(8, 0, 4)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        static const int mIndex[] = { 8, 9,10,11, 0, 1, 2, 3, 4, 5, 6, 7 };
        static const int iIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };

        for (int i = 0; i < lengthOf(mIndex); ++i) {
            QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", mIndex[i]);
            QVERIFY(delegate);
            QCOMPARE(delegate->property("test1").toString(), model.list.at(mIndex[i]));
            QCOMPARE(delegate->property("test2").toInt(), mIndex[i]);
            QCOMPARE(delegate->property("test3").toInt(), iIndex[i]);
        }
    } {
        evaluate<void>(visualModel, "items.move(3, 4, 5)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        static const int mIndex[] = { 8, 9,10,4, 11, 0, 1, 2, 3, 5, 6, 7 };
        static const int iIndex[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };

        for (int i = 0; i < lengthOf(mIndex); ++i) {
            QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", mIndex[i]);
            QVERIFY(delegate);
            QCOMPARE(delegate->property("test1").toString(), model.list.at(mIndex[i]));
            QCOMPARE(delegate->property("test2").toInt(), mIndex[i]);
            QCOMPARE(delegate->property("test3").toInt(), iIndex[i]);
        }
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: move: invalid count");
        evaluate<void>(visualModel, "items.move(5, 2, -2)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: move: from index out of range");
        evaluate<void>(visualModel, "items.move(-6, 2, 1)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: move: from index out of range");
        evaluate<void>(visualModel, "items.move(15, 2, 1)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: move: from index out of range");
        evaluate<void>(visualModel, "items.move(11, 1, 3)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: move: to index out of range");
        evaluate<void>(visualModel, "items.move(2, -5, 1)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: move: to index out of range");
        evaluate<void>(visualModel, "items.move(2, 14, 1)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: move: to index out of range");
        evaluate<void>(visualModel, "items.move(2, 11, 4)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
    }
}


template <int N> void tst_qsgvisualdatamodel::groups_verify(
        const SingleRoleModel &model,
        QSGItem *contentItem,
        const int (&mIndex)[N],
        const int (&iIndex)[N],
        const int (&vIndex)[N],
        const int (&sIndex)[N],
        const bool (&vMember)[N],
        const bool (&sMember)[N])
{
    failed = true;
    for (int i = 0; i < N; ++i) {
        QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", mIndex[i]);
        QVERIFY(delegate);
        QCOMPARE(delegate->property("test1").toString(), model.list.at(mIndex[i]));
        QCOMPARE(delegate->property("test2").toInt() , mIndex[i]);
        QCOMPARE(delegate->property("test3").toInt() , iIndex[i]);
        QCOMPARE(delegate->property("test4").toBool(), true);
        QCOMPARE(delegate->property("test5").toInt() , vIndex[i]);
        QCOMPARE(delegate->property("test6").toBool(), vMember[i]);
        QCOMPARE(delegate->property("test7").toInt() , sIndex[i]);
        QCOMPARE(delegate->property("test8").toBool(), sMember[i]);
        QCOMPARE(delegate->property("test9").toStringList().contains("items")   , QBool(true));
        QCOMPARE(delegate->property("test9").toStringList().contains("visible") , QBool(vMember[i]));
        QCOMPARE(delegate->property("test9").toStringList().contains("selected"), QBool(sMember[i]));
    }
    failed = false;
}

#define VERIFY_GROUPS \
    groups_verify(model, contentItem, mIndex, iIndex, vIndex, sIndex, vMember, sMember); \
    QVERIFY(!failed)


void tst_qsgvisualdatamodel::groups()
{
    QSGView view;

    SingleRoleModel model;
    model.list = QStringList()
            << "one"
            << "two"
            << "three"
            << "four"
            << "five"
            << "six"
            << "seven"
            << "eight"
            << "nine"
            << "ten"
            << "eleven"
            << "twelve";

    QDeclarativeContext *ctxt = view.rootContext();
    ctxt->setContextProperty("myModel", &model);

    view.setSource(QUrl::fromLocalFile(SRCDIR "/data/groups.qml"));

    QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
    QVERIFY(listview != 0);

    QSGItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    QSGVisualDataModel *visualModel = qobject_cast<QSGVisualDataModel *>(qvariant_cast<QObject *>(listview->model()));
    QVERIFY(visualModel);

    QSGVisualDataGroup *visibleItems = visualModel->findChild<QSGVisualDataGroup *>("visibleItems");
    QVERIFY(visibleItems);

    QSGVisualDataGroup *selectedItems = visualModel->findChild<QSGVisualDataGroup *>("selectedItems");
    QVERIFY(selectedItems);

    const bool f = false;
    const bool t = true;

    {
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 12);
        QCOMPARE(selectedItems->count(), 0);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const bool vMember[] = { t, t, t, t, t, t, t, t, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        static const bool sMember[] = { f, f, f, f, f, f, f, f, f, f, f, f };
        VERIFY_GROUPS;
    } {
        evaluate<void>(visualModel, "items.addGroups(8, \"selected\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 12);
        QCOMPARE(selectedItems->count(), 1);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const bool vMember[] = { t, t, t, t, t, t, t, t, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
        static const bool sMember[] = { f, f, f, f, f, f, f, f, t, f, f, f };
        VERIFY_GROUPS;
    } {
        evaluate<void>(visualModel, "items.addGroups(6, 4, [\"visible\", \"selected\"])");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 12);
        QCOMPARE(selectedItems->count(), 4);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const bool vMember[] = { t, t, t, t, t, t, t, t, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 4 };
        static const bool sMember[] = { f, f, f, f, f, f, t, t, t, t, f, f };
        VERIFY_GROUPS;
    } {
        evaluate<void>(visualModel, "items.setGroups(2, [\"items\", \"selected\"])");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 11);
        QCOMPARE(selectedItems->count(), 5);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9,10 };
        static const bool vMember[] = { t, t, f, t, t, t, t, t, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 1, 1, 1, 1, 2, 3, 4, 5, 5 };
        static const bool sMember[] = { f, f, t, f, f, f, t, t, t, t, f, f };
        VERIFY_GROUPS;
    } {
        evaluate<void>(selectedItems, "setGroups(0, 3, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 3, 4, 5, 5, 5, 6, 7, 8 };
        static const bool vMember[] = { t, t, f, t, t, t, f, f, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2 };
        static const bool sMember[] = { f, f, f, f, f, f, f, f, t, t, f, f };
        VERIFY_GROUPS;
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: addGroups: invalid count");
        evaluate<void>(visualModel, "items.addGroups(11, -4, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: addGroups: index out of range");
        evaluate<void>(visualModel, "items.addGroups(-1, 3, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: addGroups: index out of range");
        evaluate<void>(visualModel, "items.addGroups(14, 3, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: addGroups: index out of range");
        evaluate<void>(visualModel, "items.addGroups(11, 5, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: setGroups: invalid count");
        evaluate<void>(visualModel, "items.setGroups(11, -4, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: setGroups: index out of range");
        evaluate<void>(visualModel, "items.setGroups(-1, 3, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: setGroups: index out of range");
        evaluate<void>(visualModel, "items.setGroups(14, 3, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: setGroups: index out of range");
        evaluate<void>(visualModel, "items.setGroups(11, 5, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: removeGroups: invalid count");
        evaluate<void>(visualModel, "items.removeGroups(11, -4, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: removeGroups: index out of range");
        evaluate<void>(visualModel, "items.removeGroups(-1, 3, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: removeGroups: index out of range");
        evaluate<void>(visualModel, "items.removeGroups(14, 3, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML VisualDataGroup: removeGroups: index out of range");
        evaluate<void>(visualModel, "items.removeGroups(11, 5, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        evaluate<void>(visualModel, "filterOnGroup = \"visible\"");
        QCOMPARE(listview->count(), 9);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        evaluate<void>(visualModel, "filterOnGroup = \"selected\"");
        QCOMPARE(listview->count(), 2);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        evaluate<void>(visualModel, "filterOnGroup = \"items\"");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
    } {
        QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", 5);
        QVERIFY(delegate);

        evaluate<void>(delegate, "hide()");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 8);
        QCOMPARE(selectedItems->count(), 2);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 3, 4, 4, 4, 4, 5, 6, 7 };
        static const bool vMember[] = { t, t, f, t, t, f, f, f, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2 };
        static const bool sMember[] = { f, f, f, f, f, f, f, f, t, t, f, f };
        VERIFY_GROUPS;
    } {
        QSGItem *delegate = findItem<QSGItem>(contentItem, "delegate", 5);
        QVERIFY(delegate);

        evaluate<void>(delegate, "select()");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 8);
        QCOMPARE(selectedItems->count(), 3);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 3, 4, 4, 4, 4, 5, 6, 7 };
        static const bool vMember[] = { t, t, f, t, t, f, f, f, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 3, 3 };
        static const bool sMember[] = { f, f, f, f, f, t, f, f, t, t, f, f };
        VERIFY_GROUPS;
    } {
        evaluate<void>(visualModel, "items.move(2, 6, 3)");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 8);
        QCOMPARE(selectedItems->count(), 3);
        static const int  mIndex [] = { 0, 1, 5, 6, 7, 8, 2, 3, 4, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 2, 2, 3, 3, 4, 5, 6, 7 };
        static const bool vMember[] = { t, t, f, f, f, t, f, t, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3 };
        static const bool sMember[] = { f, f, t, f, f, t, f, f, f, t, f, f };
        VERIFY_GROUPS;
    }
}

template <int N> void tst_qsgvisualdatamodel::get_verify(
        const SingleRoleModel &model,
        QSGVisualDataModel *visualModel,
        QSGVisualDataGroup *visibleItems,
        QSGVisualDataGroup *selectedItems,
        const int (&mIndex)[N],
        const int (&iIndex)[N],
        const int (&vIndex)[N],
        const int (&sIndex)[N],
        const bool (&vMember)[N],
        const bool (&sMember)[N])
{
    failed = true;
    for (int i = 0; i < N; ++i) {
        QCOMPARE(evaluate<QString>(visualModel, QString("items.get(%1).model.name").arg(i)), model.list.at(mIndex[i]));
        QCOMPARE(evaluate<QString>(visualModel, QString("items.get(%1).model.modelData").arg(i)), model.list.at(mIndex[i]));
        QCOMPARE(evaluate<int>(visualModel, QString("items.get(%1).model.index").arg(i)), mIndex[i]);
        QCOMPARE(evaluate<int>(visualModel, QString("items.get(%1).itemsIndex").arg(i)), iIndex[i]);
        QCOMPARE(evaluate<bool>(visualModel, QString("items.get(%1).inItems").arg(i)), true);
        QCOMPARE(evaluate<int>(visualModel, QString("items.get(%1).visibleIndex").arg(i)), vIndex[i]);
        QCOMPARE(evaluate<bool>(visualModel, QString("items.get(%1).inVisible").arg(i)), vMember[i]);
        QCOMPARE(evaluate<int>(visualModel, QString("items.get(%1).selectedIndex").arg(i)), sIndex[i]);
        QCOMPARE(evaluate<bool>(visualModel, QString("items.get(%1).inSelected").arg(i)), sMember[i]);
        QCOMPARE(evaluate<bool>(visualModel, QString("contains(items.get(%1).groups, \"items\")").arg(i)), true);
        QCOMPARE(evaluate<bool>(visualModel, QString("contains(items.get(%1).groups, \"visible\")").arg(i)), vMember[i]);
        QCOMPARE(evaluate<bool>(visualModel, QString("contains(items.get(%1).groups, \"selected\")").arg(i)), sMember[i]);

        if (vMember[i]) {
            QCOMPARE(evaluate<QString>(visibleItems, QString("get(%1).model.name").arg(vIndex[i])), model.list.at(mIndex[i]));
            QCOMPARE(evaluate<QString>(visibleItems, QString("get(%1).model.modelData").arg(vIndex[i])), model.list.at(mIndex[i]));
            QCOMPARE(evaluate<int>(visibleItems, QString("get(%1).model.index").arg(vIndex[i])), mIndex[i]);
            QCOMPARE(evaluate<int>(visibleItems, QString("get(%1).itemsIndex").arg(vIndex[i])), iIndex[i]);
            QCOMPARE(evaluate<bool>(visibleItems, QString("get(%1).inItems").arg(vIndex[i])), true);
            QCOMPARE(evaluate<int>(visibleItems, QString("get(%1).visibleIndex").arg(vIndex[i])), vIndex[i]);
            QCOMPARE(evaluate<bool>(visibleItems, QString("get(%1).inVisible").arg(vIndex[i])), vMember[i]);
            QCOMPARE(evaluate<int>(visibleItems, QString("get(%1).selectedIndex").arg(vIndex[i])), sIndex[i]);
            QCOMPARE(evaluate<bool>(visibleItems, QString("get(%1).inSelected").arg(vIndex[i])), sMember[i]);

            QCOMPARE(evaluate<bool>(visibleItems, QString("contains(get(%1).groups, \"items\")").arg(vIndex[i])), true);
            QCOMPARE(evaluate<bool>(visibleItems, QString("contains(get(%1).groups, \"visible\")").arg(vIndex[i])), vMember[i]);
            QCOMPARE(evaluate<bool>(visibleItems, QString("contains(get(%1).groups, \"selected\")").arg(vIndex[i])), sMember[i]);
        }
        if (sMember[i]) {
            QCOMPARE(evaluate<QString>(selectedItems, QString("get(%1).model.name").arg(sIndex[i])), model.list.at(mIndex[i]));
            QCOMPARE(evaluate<QString>(selectedItems, QString("get(%1).model.modelData").arg(sIndex[i])), model.list.at(mIndex[i]));
            QCOMPARE(evaluate<int>(selectedItems, QString("get(%1).model.index").arg(sIndex[i])), mIndex[i]);
            QCOMPARE(evaluate<int>(selectedItems, QString("get(%1).itemsIndex").arg(sIndex[i])), iIndex[i]);
            QCOMPARE(evaluate<bool>(selectedItems, QString("get(%1).inItems").arg(sIndex[i])), true);
            QCOMPARE(evaluate<int>(selectedItems, QString("get(%1).visibleIndex").arg(sIndex[i])), vIndex[i]);
            QCOMPARE(evaluate<bool>(selectedItems, QString("get(%1).inVisible").arg(sIndex[i])), vMember[i]);
            QCOMPARE(evaluate<int>(selectedItems, QString("get(%1).selectedIndex").arg(sIndex[i])), sIndex[i]);
            QCOMPARE(evaluate<bool>(selectedItems, QString("get(%1).inSelected").arg(sIndex[i])), sMember[i]);
            QCOMPARE(evaluate<bool>(selectedItems, QString("contains(get(%1).groups, \"items\")").arg(sIndex[i])), true);
            QCOMPARE(evaluate<bool>(selectedItems, QString("contains(get(%1).groups, \"visible\")").arg(sIndex[i])), vMember[i]);
            QCOMPARE(evaluate<bool>(selectedItems, QString("contains(get(%1).groups, \"selected\")").arg(sIndex[i])), sMember[i]);
        }
    }
    failed = false;
}

#define VERIFY_GET \
    get_verify(model, visualModel, visibleItems, selectedItems, mIndex, iIndex, vIndex, sIndex, vMember, sMember); \
    QVERIFY(!failed)

void tst_qsgvisualdatamodel::get()
{
    QSGView view;

    SingleRoleModel model;
    model.list = QStringList()
            << "one"
            << "two"
            << "three"
            << "four"
            << "five"
            << "six"
            << "seven"
            << "eight"
            << "nine"
            << "ten"
            << "eleven"
            << "twelve";

    QDeclarativeContext *ctxt = view.rootContext();
    ctxt->setContextProperty("myModel", &model);

    view.setSource(QUrl::fromLocalFile(SRCDIR "/data/groups.qml"));

    QSGListView *listview = qobject_cast<QSGListView*>(view.rootObject());
    QVERIFY(listview != 0);

    QSGItem *contentItem = listview->contentItem();
    QVERIFY(contentItem != 0);

    QSGVisualDataModel *visualModel = qobject_cast<QSGVisualDataModel *>(qvariant_cast<QObject *>(listview->model()));
    QVERIFY(visualModel);

    QSGVisualDataGroup *visibleItems = visualModel->findChild<QSGVisualDataGroup *>("visibleItems");
    QVERIFY(visibleItems);

    QSGVisualDataGroup *selectedItems = visualModel->findChild<QSGVisualDataGroup *>("selectedItems");
    QVERIFY(selectedItems);

    QV8Engine *v8Engine = QDeclarativeEnginePrivate::getV8Engine(ctxt->engine());
    QVERIFY(v8Engine);

    const bool f = false;
    const bool t = true;

    {
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 12);
        QCOMPARE(selectedItems->count(), 0);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const bool vMember[] = { t, t, t, t, t, t, t, t, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        static const bool sMember[] = { f, f, f, f, f, f, f, f, f, f, f, f };
        VERIFY_GET;
    } {
        evaluate<void>(visualModel, "items.addGroups(8, \"selected\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 12);
        QCOMPARE(selectedItems->count(), 1);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const bool vMember[] = { t, t, t, t, t, t, t, t, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
        static const bool sMember[] = { f, f, f, f, f, f, f, f, t, f, f, f };
        VERIFY_GET;
    } {
        evaluate<void>(visualModel, "items.addGroups(6, 4, [\"visible\", \"selected\"])");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 12);
        QCOMPARE(selectedItems->count(), 4);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const bool vMember[] = { t, t, t, t, t, t, t, t, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 4 };
        static const bool sMember[] = { f, f, f, f, f, f, t, t, t, t, f, f };
        VERIFY_GET;
    } {
        evaluate<void>(visualModel, "items.setGroups(2, [\"items\", \"selected\"])");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 11);
        QCOMPARE(selectedItems->count(), 5);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9,10 };
        static const bool vMember[] = { t, t, f, t, t, t, t, t, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 1, 1, 1, 1, 2, 3, 4, 5, 5 };
        static const bool sMember[] = { f, f, t, f, f, f, t, t, t, t, f, f };
        VERIFY_GET;
    } {
        evaluate<void>(selectedItems, "setGroups(0, 3, \"items\")");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 3, 4, 5, 5, 5, 6, 7, 8 };
        static const bool vMember[] = { t, t, f, t, t, t, f, f, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2 };
        static const bool sMember[] = { f, f, f, f, f, f, f, f, t, t, f, f };
        VERIFY_GET;
    } {
        evaluate<void>(visualModel, "items.get(5).inVisible = false");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 8);
        QCOMPARE(selectedItems->count(), 2);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 3, 4, 4, 4, 4, 5, 6, 7 };
        static const bool vMember[] = { t, t, f, t, t, f, f, f, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2 };
        static const bool sMember[] = { f, f, f, f, f, f, f, f, t, t, f, f };
        VERIFY_GET;
    } {
        evaluate<void>(visualModel, "items.get(5).inSelected = true");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 8);
        QCOMPARE(selectedItems->count(), 3);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 3, 4, 4, 4, 4, 5, 6, 7 };
        static const bool vMember[] = { t, t, f, t, t, f, f, f, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 3, 3 };
        static const bool sMember[] = { f, f, f, f, f, t, f, f, t, t, f, f };
        VERIFY_GET;
    } {
        evaluate<void>(visualModel, "items.get(5).groups = [\"visible\", \"items\"]");
        QCOMPARE(listview->count(), 12);
        QCOMPARE(visualModel->items()->count(), 12);
        QCOMPARE(visibleItems->count(), 9);
        QCOMPARE(selectedItems->count(), 2);
        static const int  mIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  iIndex [] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11 };
        static const int  vIndex [] = { 0, 1, 2, 2, 3, 4, 5, 5, 5, 6, 7, 8 };
        static const bool vMember[] = { t, t, f, t, t, t, f, f, t, t, t, t };
        static const int  sIndex [] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2 };
        static const bool sMember[] = { f, f, f, f, f, f, f, f, t, t, f, f };
        VERIFY_GET;
    }
}

template<typename T>
T *tst_qsgvisualdatamodel::findItem(QSGItem *parent, const QString &objectName, int index)
{
    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->childItems().count() << "children";
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QSGItem *item = qobject_cast<QSGItem*>(parent->childItems().at(i));
        if(!item)
            continue;
        //qDebug() << "try" << item;
        if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName)) {
            if (index != -1) {
                QDeclarativeExpression e(qmlContext(item), item, "index");
                if (e.evaluate().toInt() == index)
                    return static_cast<T*>(item);
            } else {
                return static_cast<T*>(item);
            }
        }
        item = findItem<T>(item, objectName, index);
        if (item)
        return static_cast<T*>(item);
    }

    return 0;
}

QTEST_MAIN(tst_qsgvisualdatamodel)

#include "tst_qsgvisualdatamodel.moc"
