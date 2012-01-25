/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include <QtTest/QtTest>
#include <QtQuick/qquickview.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativeincubator.h>
#include <QtQuick/private/qquickpathview_p.h>
#include <QtQuick/private/qdeclarativepath_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtDeclarative/private/qdeclarativelistmodel_p.h>
#include <QtDeclarative/private/qdeclarativevaluetype_p.h>
#include <QAbstractListModel>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QFile>

#include "../../shared/util.h"

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


class tst_QQuickPathView : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_QQuickPathView();

private slots:
    void initValues();
    void items();
    void dataModel();
    void pathview2();
    void pathview3();
    void insertModel_data();
    void insertModel();
    void removeModel_data();
    void removeModel();
    void moveModel_data();
    void moveModel();
    void path();
    void pathMoved();
    void setCurrentIndex();
    void resetModel();
    void propertyChanges();
    void pathChanges();
    void componentChanges();
    void modelChanges();
    void pathUpdateOnStartChanged();
    void package();
    void emptyModel();
    void closed();
    void pathUpdate();
    void visualDataModel();
    void undefinedPath();
    void mouseDrag();
    void treeModel();
    void changePreferredHighlight();
    void missingPercent();
    void creationContext();
    void currentOffsetOnInsertion();
    void asynchronous();

private:
    QQuickView *createView();
    template<typename T>
    T *findItem(QQuickItem *parent, const QString &objectName, int index=-1);
    template<typename T>
    QList<T*> findItems(QQuickItem *parent, const QString &objectName);
};

class TestObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool error READ error WRITE setError)
    Q_PROPERTY(bool useModel READ useModel NOTIFY useModelChanged)
    Q_PROPERTY(int pathItemCount READ pathItemCount NOTIFY pathItemCountChanged)

public:
    TestObject() : QObject(), mError(true), mUseModel(true), mPathItemCount(-1) {}

    bool error() const { return mError; }
    void setError(bool err) { mError = err; }

    bool useModel() const { return mUseModel; }
    void setUseModel(bool use) { mUseModel = use; emit useModelChanged(); }

    int pathItemCount() const { return mPathItemCount; }
    void setPathItemCount(int count) { mPathItemCount = count; emit pathItemCountChanged(); }

signals:
    void useModelChanged();
    void pathItemCountChanged();

private:
    bool mError;
    bool mUseModel;
    int mPathItemCount;
};

class TestModel : public QAbstractListModel
{
public:
    enum Roles { Name = Qt::UserRole+1, Number = Qt::UserRole+2 };

    TestModel(QObject *parent=0) : QAbstractListModel(parent) {
        QHash<int, QByteArray> roles;
        roles[Name] = "name";
        roles[Number] = "number";
        setRoleNames(roles);
    }

    int rowCount(const QModelIndex &parent=QModelIndex()) const { Q_UNUSED(parent); return list.count(); }
    QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const {
        QVariant rv;
        if (role == Name)
            rv = list.at(index.row()).first;
        else if (role == Number)
            rv = list.at(index.row()).second;

        return rv;
    }

    int count() const { return rowCount(); }
    QString name(int index) const { return list.at(index).first; }
    QString number(int index) const { return list.at(index).second; }

    void addItem(const QString &name, const QString &number) {
        beginInsertRows(QModelIndex(), list.count(), list.count());
        list.append(QPair<QString,QString>(name, number));
        endInsertRows();
    }

    void insertItem(int index, const QString &name, const QString &number) {
        beginInsertRows(QModelIndex(), index, index);
        list.insert(index, QPair<QString,QString>(name, number));
        endInsertRows();
    }

    void insertItems(int index, const QList<QPair<QString, QString> > &items) {
        beginInsertRows(QModelIndex(), index, index+items.count()-1);
        for (int i=0; i<items.count(); i++)
            list.insert(index + i, QPair<QString,QString>(items[i].first, items[i].second));
        endInsertRows();
    }

    void removeItem(int index) {
        beginRemoveRows(QModelIndex(), index, index);
        list.removeAt(index);
        endRemoveRows();
    }

    void removeItems(int index, int count) {
        emit beginRemoveRows(QModelIndex(), index, index+count-1);
        while (count--)
            list.removeAt(index);
        emit endRemoveRows();
    }

    void moveItem(int from, int to) {
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        list.move(from, to);
        endMoveRows();
    }

    void moveItems(int from, int to, int count) {
        beginMoveRows(QModelIndex(), from, from+count-1, QModelIndex(), to > from ? to+count : to);
        move(from, to, count);
        endMoveRows();
    }

    void modifyItem(int idx, const QString &name, const QString &number) {
        list[idx] = QPair<QString,QString>(name, number);
        emit dataChanged(index(idx,0), index(idx,0));
    }

    void move(int from, int to, int n)
    {
        if (from > to) {
            // Only move forwards - flip if backwards moving
            int tfrom = from;
            int tto = to;
            from = tto;
            to = tto+n;
            n = tfrom-tto;
        }
        if (n == 1) {
            list.move(from, to);
        } else {
            QList<QPair<QString,QString> > replaced;
            int i=0;
            QList<QPair<QString,QString> >::ConstIterator it=list.begin(); it += from+n;
            for (; i<to-from; ++i,++it)
                replaced.append(*it);
            i=0;
            it=list.begin(); it += from;
            for (; i<n; ++i,++it)
                replaced.append(*it);
            QList<QPair<QString,QString> >::ConstIterator f=replaced.begin();
            QList<QPair<QString,QString> >::Iterator t=list.begin(); t += from;
            for (; f != replaced.end(); ++f, ++t)
                *t = *f;
        }
    }

private:
    QList<QPair<QString,QString> > list;
};


tst_QQuickPathView::tst_QQuickPathView()
{
}

void tst_QQuickPathView::initValues()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("pathview1.qml"));
    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());

    QVERIFY(obj != 0);
    QVERIFY(obj->path() == 0);
    QVERIFY(obj->delegate() == 0);
    QCOMPARE(obj->model(), QVariant());
    QCOMPARE(obj->currentIndex(), 0);
    QCOMPARE(obj->offset(), 0.);
    QCOMPARE(obj->preferredHighlightBegin(), 0.);
    QCOMPARE(obj->dragMargin(), 0.);
    QCOMPARE(obj->count(), 0);
    QCOMPARE(obj->pathItemCount(), -1);

    delete obj;
}

void tst_QQuickPathView::items()
{
    QQuickView *canvas = createView();

    TestModel model;
    model.addItem("Fred", "12345");
    model.addItem("John", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    QCOMPARE(pathview->count(), model.count());
    QCOMPARE(canvas->rootObject()->property("count").toInt(), model.count());
    QCOMPARE(pathview->childItems().count(), model.count()+1); // assumes all are visible, including highlight

    for (int i = 0; i < model.count(); ++i) {
        QQuickText *name = findItem<QQuickText>(pathview, "textName", i);
        QVERIFY(name != 0);
        QCOMPARE(name->text(), model.name(i));
        QQuickText *number = findItem<QQuickText>(pathview, "textNumber", i);
        QVERIFY(number != 0);
        QCOMPARE(number->text(), model.number(i));
    }

    QDeclarativePath *path = qobject_cast<QDeclarativePath*>(pathview->path());
    QVERIFY(path);

    QVERIFY(pathview->highlightItem());
    QPointF start = path->pointAt(0.0);
    QPointF offset;
    offset.setX(pathview->highlightItem()->width()/2);
    offset.setY(pathview->highlightItem()->height()/2);
    QCOMPARE(pathview->highlightItem()->pos() + offset, start);

    delete canvas;
}

void tst_QQuickPathView::pathview2()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("pathview2.qml"));
    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());

    QVERIFY(obj != 0);
    QVERIFY(obj->path() != 0);
    QVERIFY(obj->delegate() != 0);
    QVERIFY(obj->model() != QVariant());
    QCOMPARE(obj->currentIndex(), 0);
    QCOMPARE(obj->offset(), 0.);
    QCOMPARE(obj->preferredHighlightBegin(), 0.);
    QCOMPARE(obj->dragMargin(), 0.);
    QCOMPARE(obj->count(), 8);
    QCOMPARE(obj->pathItemCount(), 10);

    delete obj;
}

void tst_QQuickPathView::pathview3()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("pathview3.qml"));
    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());

    QVERIFY(obj != 0);
    QVERIFY(obj->path() != 0);
    QVERIFY(obj->delegate() != 0);
    QVERIFY(obj->model() != QVariant());
    QCOMPARE(obj->currentIndex(), 0);
    QCOMPARE(obj->offset(), 1.0);
    QCOMPARE(obj->preferredHighlightBegin(), 0.5);
    QCOMPARE(obj->dragMargin(), 24.);
    QCOMPARE(obj->count(), 8);
    QCOMPARE(obj->pathItemCount(), 4);

    delete obj;
}

void tst_QQuickPathView::insertModel_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("idx");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("offset");

    // We have 8 items, with currentIndex == 4
    QTest::newRow("insert after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 6 << 1 << 5.;
    QTest::newRow("insert before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 1 << 4.;
    QTest::newRow("insert multiple after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 5 << 2 << 6.;
    QTest::newRow("insert multiple before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 1 << 2 << 4.;
    QTest::newRow("insert at end")
        << int(QQuickPathView::StrictlyEnforceRange) << 8 << 1 << 5.;
    QTest::newRow("insert at beginning")
        << int(QQuickPathView::StrictlyEnforceRange) << 0 << 1 << 4.;
    QTest::newRow("insert at current")
        << int(QQuickPathView::StrictlyEnforceRange) << 4 << 1 << 4.;

    QTest::newRow("no range - insert after current")
        << int(QQuickPathView::NoHighlightRange) << 6 << 1 << 5.;
    QTest::newRow("no range - insert before current")
        << int(QQuickPathView::NoHighlightRange) << 2 << 1 << 4.;
    QTest::newRow("no range - insert multiple after current")
        << int(QQuickPathView::NoHighlightRange) << 5 << 2 << 6.;
    QTest::newRow("no range - insert multiple before current")
        << int(QQuickPathView::NoHighlightRange) << 1 << 2 << 4.;
    QTest::newRow("no range - insert at end")
        << int(QQuickPathView::NoHighlightRange) << 8 << 1 << 5.;
    QTest::newRow("no range - insert at beginning")
        << int(QQuickPathView::NoHighlightRange) << 0 << 1 << 4.;
    QTest::newRow("no range - insert at current")
        << int(QQuickPathView::NoHighlightRange) << 4 << 1 << 4.;
}

void tst_QQuickPathView::insertModel()
{
    QFETCH(int, mode);
    QFETCH(int, idx);
    QFETCH(int, count);
    QFETCH(qreal, offset);

    QQuickView *canvas = createView();
    canvas->show();

    TestModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");
    model.addItem("Jinny", "679");
    model.addItem("Milly", "73378");
    model.addItem("Jimmy", "3535");
    model.addItem("Barb", "9039");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    pathview->setHighlightRangeMode((QQuickPathView::HighlightRangeMode)mode);

    pathview->setCurrentIndex(4);
    if (mode == QQuickPathView::StrictlyEnforceRange)
        QTRY_COMPARE(pathview->offset(), 4.0);
    else
        pathview->setOffset(4);

    QList<QPair<QString, QString> > items;
    for (int i = 0; i < count; ++i)
        items.append(qMakePair(QString("New"), QString::number(i)));

    model.insertItems(idx, items);
    QTRY_COMPARE(pathview->offset(), offset);

    delete canvas;
}

void tst_QQuickPathView::removeModel_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("idx");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("offset");

    // We have 8 items, with currentIndex == 4
    QTest::newRow("remove after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 6 << 1 << 3.;
    QTest::newRow("remove before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 1 << 4.;
    QTest::newRow("remove multiple after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 5 << 2 << 2.;
    QTest::newRow("remove multiple before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 1 << 2 << 4.;
    QTest::newRow("remove last")
        << int(QQuickPathView::StrictlyEnforceRange) << 7 << 1 << 3.;
    QTest::newRow("remove first")
        << int(QQuickPathView::StrictlyEnforceRange) << 0 << 1 << 4.;
    QTest::newRow("remove current")
        << int(QQuickPathView::StrictlyEnforceRange) << 4 << 1 << 3.;

    QTest::newRow("no range - remove after current")
        << int(QQuickPathView::NoHighlightRange) << 6 << 1 << 3.;
    QTest::newRow("no range - remove before current")
        << int(QQuickPathView::NoHighlightRange) << 2 << 1 << 4.;
    QTest::newRow("no range - remove multiple after current")
        << int(QQuickPathView::NoHighlightRange) << 5 << 2 << 2.;
    QTest::newRow("no range - remove multiple before current")
        << int(QQuickPathView::NoHighlightRange) << 1 << 2 << 4.;
    QTest::newRow("no range - remove last")
        << int(QQuickPathView::NoHighlightRange) << 7 << 1 << 3.;
    QTest::newRow("no range - remove first")
        << int(QQuickPathView::NoHighlightRange) << 0 << 1 << 4.;
    QTest::newRow("no range - remove current offset")
        << int(QQuickPathView::NoHighlightRange) << 4 << 1 << 4.;
}

void tst_QQuickPathView::removeModel()
{
    QFETCH(int, mode);
    QFETCH(int, idx);
    QFETCH(int, count);
    QFETCH(qreal, offset);

    QQuickView *canvas = createView();
    canvas->show();

    TestModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");
    model.addItem("Jinny", "679");
    model.addItem("Milly", "73378");
    model.addItem("Jimmy", "3535");
    model.addItem("Barb", "9039");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    pathview->setHighlightRangeMode((QQuickPathView::HighlightRangeMode)mode);

    pathview->setCurrentIndex(4);
    if (mode == QQuickPathView::StrictlyEnforceRange)
        QTRY_COMPARE(pathview->offset(), 4.0);
    else
        pathview->setOffset(4);

    model.removeItems(idx, count);
    QTRY_COMPARE(pathview->offset(), offset);

    delete canvas;
}


void tst_QQuickPathView::moveModel_data()
{
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::addColumn<int>("count");
    QTest::addColumn<qreal>("offset");

    // We have 8 items, with currentIndex == 4
    QTest::newRow("move after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 5 << 6 << 1 << 4.;
    QTest::newRow("move before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 3 << 1 << 4.;
    QTest::newRow("move before current to after")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 6 << 1 << 5.;
    QTest::newRow("move multiple after current")
        << int(QQuickPathView::StrictlyEnforceRange) << 5 << 6 << 2 << 4.;
    QTest::newRow("move multiple before current")
        << int(QQuickPathView::StrictlyEnforceRange) << 0 << 1 << 2 << 4.;
    QTest::newRow("move before current to end")
        << int(QQuickPathView::StrictlyEnforceRange) << 2 << 7 << 1 << 5.;
    QTest::newRow("move last to beginning")
        << int(QQuickPathView::StrictlyEnforceRange) << 7 << 0 << 1 << 3.;
    QTest::newRow("move current")
        << int(QQuickPathView::StrictlyEnforceRange) << 4 << 6 << 1 << 2.;

    QTest::newRow("no range - move after current")
        << int(QQuickPathView::NoHighlightRange) << 5 << 6 << 1 << 4.;
    QTest::newRow("no range - move before current")
        << int(QQuickPathView::NoHighlightRange) << 2 << 3 << 1 << 4.;
    QTest::newRow("no range - move before current to after")
        << int(QQuickPathView::NoHighlightRange) << 2 << 6 << 1 << 5.;
    QTest::newRow("no range - move multiple after current")
        << int(QQuickPathView::NoHighlightRange) << 5 << 6 << 2 << 4.;
    QTest::newRow("no range - move multiple before current")
        << int(QQuickPathView::NoHighlightRange) << 0 << 1 << 2 << 4.;
    QTest::newRow("no range - move before current to end")
        << int(QQuickPathView::NoHighlightRange) << 2 << 7 << 1 << 5.;
    QTest::newRow("no range - move last to beginning")
        << int(QQuickPathView::NoHighlightRange) << 7 << 0 << 1 << 3.;
    QTest::newRow("no range - move current")
        << int(QQuickPathView::NoHighlightRange) << 4 << 6 << 1 << 4.;
    QTest::newRow("no range - move multiple incl. current")
        << int(QQuickPathView::NoHighlightRange) << 0 << 1 << 5 << 4.;
}

void tst_QQuickPathView::moveModel()
{
    QFETCH(int, mode);
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(int, count);
    QFETCH(qreal, offset);

    QQuickView *canvas = createView();
    canvas->show();

    TestModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");
    model.addItem("Jinny", "679");
    model.addItem("Milly", "73378");
    model.addItem("Jimmy", "3535");
    model.addItem("Barb", "9039");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    pathview->setHighlightRangeMode((QQuickPathView::HighlightRangeMode)mode);

    pathview->setCurrentIndex(4);
    if (mode == QQuickPathView::StrictlyEnforceRange)
        QTRY_COMPARE(pathview->offset(), 4.0);
    else
        pathview->setOffset(4);

    model.moveItems(from, to, count);
    QTRY_COMPARE(pathview->offset(), offset);

    delete canvas;
}

void tst_QQuickPathView::path()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("pathtest.qml"));
    QDeclarativePath *obj = qobject_cast<QDeclarativePath*>(c.create());

    QVERIFY(obj != 0);
    QCOMPARE(obj->startX(), 120.);
    QCOMPARE(obj->startY(), 100.);
    QVERIFY(obj->path() != QPainterPath());

    QDeclarativeListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 5);

    QDeclarativePathAttribute* attr = qobject_cast<QDeclarativePathAttribute*>(list.at(0));
    QVERIFY(attr != 0);
    QCOMPARE(attr->name(), QString("scale"));
    QCOMPARE(attr->value(), 1.0);

    QDeclarativePathQuad* quad = qobject_cast<QDeclarativePathQuad*>(list.at(1));
    QVERIFY(quad != 0);
    QCOMPARE(quad->x(), 120.);
    QCOMPARE(quad->y(), 25.);
    QCOMPARE(quad->controlX(), 260.);
    QCOMPARE(quad->controlY(), 75.);

    QDeclarativePathPercent* perc = qobject_cast<QDeclarativePathPercent*>(list.at(2));
    QVERIFY(perc != 0);
    QCOMPARE(perc->value(), 0.3);

    QDeclarativePathLine* line = qobject_cast<QDeclarativePathLine*>(list.at(3));
    QVERIFY(line != 0);
    QCOMPARE(line->x(), 120.);
    QCOMPARE(line->y(), 100.);

    QDeclarativePathCubic* cubic = qobject_cast<QDeclarativePathCubic*>(list.at(4));
    QVERIFY(cubic != 0);
    QCOMPARE(cubic->x(), 180.);
    QCOMPARE(cubic->y(), 0.);
    QCOMPARE(cubic->control1X(), -10.);
    QCOMPARE(cubic->control1Y(), 90.);
    QCOMPARE(cubic->control2X(), 210.);
    QCOMPARE(cubic->control2Y(), 90.);

    delete obj;
}

void tst_QQuickPathView::dataModel()
{
    QQuickView *canvas = createView();
    canvas->show();

    QDeclarativeContext *ctxt = canvas->rootContext();
    TestObject *testObject = new TestObject;
    ctxt->setContextProperty("testObject", testObject);

    TestModel model;
    model.addItem("red", "1");
    model.addItem("green", "2");
    model.addItem("blue", "3");
    model.addItem("purple", "4");
    model.addItem("gray", "5");
    model.addItem("brown", "6");
    model.addItem("yellow", "7");
    model.addItem("thistle", "8");
    model.addItem("cyan", "9");
    model.addItem("peachpuff", "10");
    model.addItem("powderblue", "11");
    model.addItem("gold", "12");
    model.addItem("sandybrown", "13");

    ctxt->setContextProperty("testData", &model);

    canvas->setSource(testFileUrl("datamodel.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QVERIFY(testObject->error() == false);

    QQuickItem *item = findItem<QQuickItem>(pathview, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->x(), 110.0);
    QCOMPARE(item->y(), 10.0);

    model.insertItem(4, "orange", "10");
    QTest::qWait(100);

    QCOMPARE(canvas->rootObject()->property("viewCount").toInt(), model.count());
    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 14);

    QVERIFY(pathview->currentIndex() == 0);
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 0));

    QQuickText *text = findItem<QQuickText>(pathview, "myText", 4);
    QVERIFY(text);
    QCOMPARE(text->text(), model.name(4));

    model.removeItem(2);
    QCOMPARE(canvas->rootObject()->property("viewCount").toInt(), model.count());
    text = findItem<QQuickText>(pathview, "myText", 2);
    QVERIFY(text);
    QCOMPARE(text->text(), model.name(2));
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 0));

    testObject->setPathItemCount(5);
    QMetaObject::invokeMethod(canvas->rootObject(), "checkProperties");
    QVERIFY(testObject->error() == false);

    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);

    QQuickRectangle *testItem = findItem<QQuickRectangle>(pathview, "wrapper", 4);
    QVERIFY(testItem != 0);
    testItem = findItem<QQuickRectangle>(pathview, "wrapper", 5);
    QVERIFY(testItem == 0);

    pathview->setCurrentIndex(1);
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 1));
    QTest::qWait(100);

    model.insertItem(2, "pink", "2");
    QTest::qWait(100);

    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);
    QVERIFY(pathview->currentIndex() == 1);
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 1));

    text = findItem<QQuickText>(pathview, "myText", 2);
    QVERIFY(text);
    QCOMPARE(text->text(), model.name(2));

    model.removeItem(3);
    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);
    text = findItem<QQuickText>(pathview, "myText", 3);
    QVERIFY(text);
    QCOMPARE(text->text(), model.name(3));
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 1));

    model.moveItem(3, 5);
    QTRY_COMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);
    QList<QQuickItem*> items = findItems<QQuickItem>(pathview, "wrapper");
    foreach (QQuickItem *item, items) {
        QVERIFY(item->property("onPath").toBool());
    }
    QCOMPARE(pathview->currentItem(), findItem<QQuickItem>(pathview, "wrapper", 1));

    // QTBUG-14199
    pathview->setOffset(7);
    pathview->setOffset(0);
    QCOMPARE(findItems<QQuickItem>(pathview, "wrapper").count(), 5);

    pathview->setCurrentIndex(model.count()-1);
    model.removeItem(model.count()-1);
    QCOMPARE(pathview->currentIndex(), model.count()-1);

    delete canvas;
    delete testObject;
}

void tst_QQuickPathView::pathMoved()
{
    QQuickView *canvas = createView();
    canvas->show();

    TestModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    QQuickRectangle *firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QDeclarativePath *path = qobject_cast<QDeclarativePath*>(pathview->path());
    QVERIFY(path);
    QPointF start = path->pointAt(0.0);
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(firstItem->width()/2);
    offset.setY(firstItem->height()/2);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    pathview->setOffset(1.0);

    for (int i=0; i<model.count(); i++) {
        QQuickRectangle *curItem = findItem<QQuickRectangle>(pathview, "wrapper", i);
        QPointF itemPos(path->pointAt(0.25 + i*0.25));
        QCOMPARE(curItem->pos() + offset, QPointF(qRound(itemPos.x()), qRound(itemPos.y())));
    }

    pathview->setOffset(0.0);
    QCOMPARE(firstItem->pos() + offset, start);

    // Change delegate size
    pathview->setOffset(0.1);
    pathview->setOffset(0.0);
    canvas->rootObject()->setProperty("delegateWidth", 30);
    QCOMPARE(firstItem->width(), 30.0);
    offset.setX(firstItem->width()/2);
    QTRY_COMPARE(firstItem->pos() + offset, start);

    // Change delegate scale
    pathview->setOffset(0.1);
    pathview->setOffset(0.0);
    canvas->rootObject()->setProperty("delegateScale", 1.2);
    QTRY_COMPARE(firstItem->pos() + offset, start);

    delete canvas;
}

void tst_QQuickPathView::setCurrentIndex()
{
    QQuickView *canvas = createView();
    canvas->show();

    TestModel model;
    model.addItem("Ben", "12345");
    model.addItem("Bohn", "2345");
    model.addItem("Bob", "54321");
    model.addItem("Bill", "4321");

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathview0.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    QQuickRectangle *firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QDeclarativePath *path = qobject_cast<QDeclarativePath*>(pathview->path());
    QVERIFY(path);
    QPointF start = path->pointAt(0.0);
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(firstItem->width()/2);
    offset.setY(firstItem->height()/2);
    QCOMPARE(firstItem->pos() + offset, start);
    QCOMPARE(canvas->rootObject()->property("currentA").toInt(), 0);
    QCOMPARE(canvas->rootObject()->property("currentB").toInt(), 0);

    pathview->setCurrentIndex(2);

    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 2);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(canvas->rootObject()->property("currentA").toInt(), 2);
    QCOMPARE(canvas->rootObject()->property("currentB").toInt(), 2);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 1);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 1);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 0);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 3);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 3);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->incrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 0);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    // move an item, set move duration to 0, and change currentIndex to moved item. QTBUG-22786
    model.moveItem(0, 3);
    pathview->setHighlightMoveDuration(0);
    pathview->setCurrentIndex(3);
    QCOMPARE(pathview->currentIndex(), 3);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 3);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    model.moveItem(3, 0);
    pathview->setCurrentIndex(0);
    pathview->setHighlightMoveDuration(300);

    // Check the current item is still created when outside the bounds of pathItemCount.
    pathview->setPathItemCount(2);
    pathview->setHighlightRangeMode(QQuickPathView::NoHighlightRange);
    QVERIFY(findItem<QQuickRectangle>(pathview, "wrapper", 0));
    QVERIFY(findItem<QQuickRectangle>(pathview, "wrapper", 1));
    QVERIFY(!findItem<QQuickRectangle>(pathview, "wrapper", 2));
    QVERIFY(!findItem<QQuickRectangle>(pathview, "wrapper", 3));

    pathview->setCurrentIndex(2);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 2);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(false));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 1);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 1);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 0);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    pathview->decrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 3);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 3);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(false));

    pathview->incrementCurrentIndex();
    QTRY_COMPARE(pathview->currentIndex(), 0);
    firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QCOMPARE(pathview->currentItem(), firstItem);
    QCOMPARE(firstItem->property("onPath"), QVariant(true));

    delete canvas;
}

void tst_QQuickPathView::resetModel()
{
    QQuickView *canvas = createView();

    QStringList strings;
    strings << "one" << "two" << "three";
    QStringListModel model(strings);

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("displaypath.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    QCOMPARE(pathview->count(), model.rowCount());

    for (int i = 0; i < model.rowCount(); ++i) {
        QQuickText *display = findItem<QQuickText>(pathview, "displayText", i);
        QVERIFY(display != 0);
        QCOMPARE(display->text(), strings.at(i));
    }

    strings.clear();
    strings << "four" << "five" << "six" << "seven";
    model.setStringList(strings);

    QCOMPARE(pathview->count(), model.rowCount());

    for (int i = 0; i < model.rowCount(); ++i) {
        QQuickText *display = findItem<QQuickText>(pathview, "displayText", i);
        QVERIFY(display != 0);
        QCOMPARE(display->text(), strings.at(i));
    }

    delete canvas;
}

void tst_QQuickPathView::propertyChanges()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("propertychanges.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QSignalSpy snapPositionSpy(pathView, SIGNAL(preferredHighlightBeginChanged()));
    QSignalSpy dragMarginSpy(pathView, SIGNAL(dragMarginChanged()));

    QCOMPARE(pathView->preferredHighlightBegin(), 0.1);
    QCOMPARE(pathView->dragMargin(), 5.0);

    pathView->setPreferredHighlightBegin(0.4);
    pathView->setPreferredHighlightEnd(0.4);
    pathView->setDragMargin(20.0);

    QCOMPARE(pathView->preferredHighlightBegin(), 0.4);
    QCOMPARE(pathView->preferredHighlightEnd(), 0.4);
    QCOMPARE(pathView->dragMargin(), 20.0);

    QCOMPARE(snapPositionSpy.count(), 1);
    QCOMPARE(dragMarginSpy.count(), 1);

    pathView->setPreferredHighlightBegin(0.4);
    pathView->setPreferredHighlightEnd(0.4);
    pathView->setDragMargin(20.0);

    QCOMPARE(snapPositionSpy.count(), 1);
    QCOMPARE(dragMarginSpy.count(), 1);
    delete canvas;
}

void tst_QQuickPathView::pathChanges()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("propertychanges.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QDeclarativePath *path = canvas->rootObject()->findChild<QDeclarativePath*>("path");
    QVERIFY(path);

    QSignalSpy startXSpy(path, SIGNAL(startXChanged()));
    QSignalSpy startYSpy(path, SIGNAL(startYChanged()));

    QCOMPARE(path->startX(), 220.0);
    QCOMPARE(path->startY(), 200.0);

    path->setStartX(240.0);
    path->setStartY(220.0);

    QCOMPARE(path->startX(), 240.0);
    QCOMPARE(path->startY(), 220.0);

    QCOMPARE(startXSpy.count(),1);
    QCOMPARE(startYSpy.count(),1);

    path->setStartX(240);
    path->setStartY(220);

    QCOMPARE(startXSpy.count(),1);
    QCOMPARE(startYSpy.count(),1);

    QDeclarativePath *alternatePath = canvas->rootObject()->findChild<QDeclarativePath*>("alternatePath");
    QVERIFY(alternatePath);

    QSignalSpy pathSpy(pathView, SIGNAL(pathChanged()));

    QCOMPARE(pathView->path(), path);

    pathView->setPath(alternatePath);
    QCOMPARE(pathView->path(), alternatePath);
    QCOMPARE(pathSpy.count(),1);

    pathView->setPath(alternatePath);
    QCOMPARE(pathSpy.count(),1);

    QDeclarativePathAttribute *pathAttribute = canvas->rootObject()->findChild<QDeclarativePathAttribute*>("pathAttribute");
    QVERIFY(pathAttribute);

    QSignalSpy nameSpy(pathAttribute, SIGNAL(nameChanged()));
    QCOMPARE(pathAttribute->name(), QString("opacity"));

    pathAttribute->setName("scale");
    QCOMPARE(pathAttribute->name(), QString("scale"));
    QCOMPARE(nameSpy.count(),1);

    pathAttribute->setName("scale");
    QCOMPARE(nameSpy.count(),1);
    delete canvas;
}

void tst_QQuickPathView::componentChanges()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("propertychanges.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QDeclarativeComponent delegateComponent(canvas->engine());
    delegateComponent.setData("import QtQuick 2.0; Text { text: '<b>Name:</b> ' + name }", QUrl::fromLocalFile(""));

    QSignalSpy delegateSpy(pathView, SIGNAL(delegateChanged()));

    pathView->setDelegate(&delegateComponent);
    QCOMPARE(pathView->delegate(), &delegateComponent);
    QCOMPARE(delegateSpy.count(),1);

    pathView->setDelegate(&delegateComponent);
    QCOMPARE(delegateSpy.count(),1);
    delete canvas;
}

void tst_QQuickPathView::modelChanges()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("propertychanges.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QDeclarativeListModel *alternateModel = canvas->rootObject()->findChild<QDeclarativeListModel*>("alternateModel");
    QVERIFY(alternateModel);
    QVariant modelVariant = QVariant::fromValue<QObject *>(alternateModel);
    QSignalSpy modelSpy(pathView, SIGNAL(modelChanged()));

    pathView->setModel(modelVariant);
    QCOMPARE(pathView->model(), modelVariant);
    QCOMPARE(modelSpy.count(),1);

    pathView->setModel(modelVariant);
    QCOMPARE(modelSpy.count(),1);

    pathView->setModel(QVariant());
    QCOMPARE(modelSpy.count(),2);

    delete canvas;
}

void tst_QQuickPathView::pathUpdateOnStartChanged()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("pathUpdateOnStartChanged.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QDeclarativePath *path = canvas->rootObject()->findChild<QDeclarativePath*>("path");
    QVERIFY(path);
    QCOMPARE(path->startX(), 400.0);
    QCOMPARE(path->startY(), 300.0);

    QQuickItem *item = findItem<QQuickItem>(pathView, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->x(), path->startX() - item->width() / 2.0);
    QCOMPARE(item->y(), path->startY() - item->height() / 2.0);

    delete canvas;
}

void tst_QQuickPathView::package()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("pathview_package.qml"));
    canvas->show();
    QTest::qWaitForWindowShown(canvas);

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("photoPathView");
    QVERIFY(pathView);

#ifdef Q_OS_MAC
    QSKIP("QTBUG-21590 view does not reliably receive polish without a running animation");
#endif

    QQuickItem *item = findItem<QQuickItem>(pathView, "pathItem");
    QVERIFY(item);
    QVERIFY(item->scale() != 1.0);

    delete canvas;
}

//QTBUG-13017
void tst_QQuickPathView::emptyModel()
{
    QQuickView *canvas = createView();

    QStringListModel model;

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("emptyModel", &model);

    canvas->setSource(testFileUrl("emptymodel.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    QCOMPARE(pathview->offset(), qreal(0.0));

    delete canvas;
}

void tst_QQuickPathView::closed()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent c(&engine, testFileUrl("openPath.qml"));
        QDeclarativePath *obj = qobject_cast<QDeclarativePath*>(c.create());
        QVERIFY(obj);
        QCOMPARE(obj->isClosed(), false);
        delete obj;
    }

    {
        QDeclarativeComponent c(&engine, testFileUrl("closedPath.qml"));
        QDeclarativePath *obj = qobject_cast<QDeclarativePath*>(c.create());
        QVERIFY(obj);
        QCOMPARE(obj->isClosed(), true);
        delete obj;
    }
}

// QTBUG-14239
void tst_QQuickPathView::pathUpdate()
{
    QQuickView *canvas = createView();
    QVERIFY(canvas);
    canvas->setSource(testFileUrl("pathUpdate.qml"));

    QQuickPathView *pathView = canvas->rootObject()->findChild<QQuickPathView*>("pathView");
    QVERIFY(pathView);

    QQuickItem *item = findItem<QQuickItem>(pathView, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->x(), 150.0);

    delete canvas;
}

void tst_QQuickPathView::visualDataModel()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("vdm.qml"));

    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());
    QVERIFY(obj != 0);

    QCOMPARE(obj->count(), 3);

    delete obj;
}

void tst_QQuickPathView::undefinedPath()
{
    QDeclarativeEngine engine;

    // QPainterPath warnings are only received if QT_NO_DEBUG is not defined
    if (QLibraryInfo::isDebugBuild()) {
        QString warning1("QPainterPath::moveTo: Adding point where x or y is NaN or Inf, ignoring call");
        QTest::ignoreMessage(QtWarningMsg,qPrintable(warning1));

        QString warning2("QPainterPath::lineTo: Adding point where x or y is NaN or Inf, ignoring call");
        QTest::ignoreMessage(QtWarningMsg,qPrintable(warning2));
    }

    QDeclarativeComponent c(&engine, testFileUrl("undefinedpath.qml"));

    QQuickPathView *obj = qobject_cast<QQuickPathView*>(c.create());
    QVERIFY(obj != 0);

    QCOMPARE(obj->count(), 3);

    delete obj;
}

void tst_QQuickPathView::mouseDrag()
{
    QQuickView *canvas = createView();
    canvas->setSource(testFileUrl("dragpath.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_COMPARE(canvas, qGuiApp->focusWindow());

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    int current = pathview->currentIndex();

    QTest::mousePress(canvas, Qt::LeftButton, 0, QPoint(10,100));
    QTest::qWait(100);

    {
        QMouseEvent mv(QEvent::MouseMove, QPoint(30,100), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QGuiApplication::sendEvent(canvas, &mv);
    }
    {
        QMouseEvent mv(QEvent::MouseMove, QPoint(90,100), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QGuiApplication::sendEvent(canvas, &mv);
    }

    QVERIFY(pathview->currentIndex() != current);

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, QPoint(40,100));

    delete canvas;
}

void tst_QQuickPathView::treeModel()
{
    QQuickView *canvas = createView();
    canvas->show();

    QStandardItemModel model;
    initStandardTreeModel(&model);
    canvas->engine()->rootContext()->setContextProperty("myModel", &model);

    canvas->setSource(testFileUrl("treemodel.qml"));

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);
    QCOMPARE(pathview->count(), 3);

    QQuickText *item = findItem<QQuickText>(pathview, "wrapper", 0);
    QVERIFY(item);
    QCOMPARE(item->text(), QLatin1String("Row 1 Item"));

    QVERIFY(QMetaObject::invokeMethod(pathview, "setRoot", Q_ARG(QVariant, 1)));
    QCOMPARE(pathview->count(), 1);

    QTRY_VERIFY(item = findItem<QQuickText>(pathview, "wrapper", 0));
    QTRY_COMPARE(item->text(), QLatin1String("Row 2 Child Item"));

    delete canvas;
}

void tst_QQuickPathView::changePreferredHighlight()
{
    QQuickView *canvas = createView();
    canvas->setGeometry(0,0,400,200);
    canvas->setSource(testFileUrl("dragpath.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_COMPARE(canvas, qGuiApp->focusWindow());

    QQuickPathView *pathview = qobject_cast<QQuickPathView*>(canvas->rootObject());
    QVERIFY(pathview != 0);

    int current = pathview->currentIndex();
    QCOMPARE(current, 0);

    QQuickRectangle *firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QDeclarativePath *path = qobject_cast<QDeclarativePath*>(pathview->path());
    QVERIFY(path);
    QPointF start = path->pointAt(0.5);
    start.setX(qRound(start.x()));
    start.setY(qRound(start.y()));
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(firstItem->width()/2);
    offset.setY(firstItem->height()/2);
    QTRY_COMPARE(firstItem->pos() + offset, start);

    pathview->setPreferredHighlightBegin(0.8);
    pathview->setPreferredHighlightEnd(0.8);
    start = path->pointAt(0.8);
    start.setX(qRound(start.x()));
    start.setY(qRound(start.y()));
    QTRY_COMPARE(firstItem->pos() + offset, start);
    QCOMPARE(pathview->currentIndex(), 0);

    delete canvas;
}

void tst_QQuickPathView::creationContext()
{
    QQuickView canvas;
    canvas.setGeometry(0,0,240,320);
    canvas.setSource(testFileUrl("creationContext.qml"));

    QQuickItem *rootItem = qobject_cast<QQuickItem *>(canvas.rootObject());
    QVERIFY(rootItem);
    QVERIFY(rootItem->property("count").toInt() > 0);

    QQuickItem *item;
    QVERIFY(item = findItem<QQuickItem>(rootItem, "listItem", 0));
    QCOMPARE(item->property("text").toString(), QString("Hello!"));
}

// QTBUG-21320
void tst_QQuickPathView::currentOffsetOnInsertion()
{
    QQuickView *canvas = createView();
    canvas->show();

    TestModel model;

    QDeclarativeContext *ctxt = canvas->rootContext();
    ctxt->setContextProperty("testModel", &model);

    canvas->setSource(testFileUrl("pathline.qml"));
    qApp->processEvents();

    QQuickPathView *pathview = findItem<QQuickPathView>(canvas->rootObject(), "view");
    QVERIFY(pathview != 0);

    pathview->setPreferredHighlightBegin(0.5);
    pathview->setPreferredHighlightEnd(0.5);

    QCOMPARE(pathview->count(), model.count());

    model.addItem("item0", "0");

    QCOMPARE(pathview->count(), model.count());

    QQuickRectangle *item = 0;
    QTRY_VERIFY(item = findItem<QQuickRectangle>(pathview, "wrapper", 0));

    QDeclarativePath *path = qobject_cast<QDeclarativePath*>(pathview->path());
    QVERIFY(path);

    QPointF start = path->pointAt(0.5);
    start = QPointF(qRound(start.x()), qRound(start.y()));
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(item->width()/2);
    offset.setY(item->height()/2);
    QCOMPARE(item->pos() + offset, start);

    QSignalSpy currentIndexSpy(pathview, SIGNAL(currentIndexChanged()));

    // insert an item at the beginning
    model.insertItem(0, "item1", "1");
    qApp->processEvents();

    QCOMPARE(currentIndexSpy.count(), 1);

    // currentIndex is now 1
    QVERIFY(item = findItem<QQuickRectangle>(pathview, "wrapper", 1));

    // verify that current item (item 1) is still at offset 0.5
    QCOMPARE(item->pos() + offset, start);

    // insert another item at the beginning
    model.insertItem(0, "item2", "2");
    qApp->processEvents();

    QCOMPARE(currentIndexSpy.count(), 2);

    // currentIndex is now 2
    QVERIFY(item = findItem<QQuickRectangle>(pathview, "wrapper", 2));

    // verify that current item (item 2) is still at offset 0.5
    QCOMPARE(item->pos() + offset, start);

    // verify that remove before current maintains current item
    model.removeItem(0);
    qApp->processEvents();

    QCOMPARE(currentIndexSpy.count(), 3);

    // currentIndex is now 1
    QVERIFY(item = findItem<QQuickRectangle>(pathview, "wrapper", 1));

    // verify that current item (item 1) is still at offset 0.5
    QCOMPARE(item->pos() + offset, start);

    delete canvas;
}

void tst_QQuickPathView::asynchronous()
{
    QQuickView *canvas = createView();
    canvas->show();
    QDeclarativeIncubationController controller;
    canvas->engine()->setIncubationController(&controller);

    canvas->setSource(testFileUrl("asyncloader.qml"));

    QQuickItem *rootObject = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(rootObject);

    QQuickPathView *pathview = 0;
    while (!pathview) {
        bool b = false;
        controller.incubateWhile(&b);
        pathview = rootObject->findChild<QQuickPathView*>("view");
    }

    // items will be created one at a time
    for (int i = 0; i < 5; ++i) {
        QVERIFY(findItem<QQuickItem>(pathview, "wrapper", i) == 0);
        QQuickItem *item = 0;
        while (!item) {
            bool b = false;
            controller.incubateWhile(&b);
            item = findItem<QQuickItem>(pathview, "wrapper", i);
        }
    }

    {
        bool b = true;
        controller.incubateWhile(&b);
    }

    // verify positioning
    QQuickRectangle *firstItem = findItem<QQuickRectangle>(pathview, "wrapper", 0);
    QVERIFY(firstItem);
    QDeclarativePath *path = qobject_cast<QDeclarativePath*>(pathview->path());
    QVERIFY(path);
    QPointF start = path->pointAt(0.0);
    QPointF offset;//Center of item is at point, but pos is from corner
    offset.setX(firstItem->width()/2);
    offset.setY(firstItem->height()/2);
    QTRY_COMPARE(firstItem->pos() + offset, start);
    pathview->setOffset(1.0);

    for (int i=0; i<5; i++) {
        QQuickItem *curItem = findItem<QQuickItem>(pathview, "wrapper", i);
        QPointF itemPos(path->pointAt(0.2 + i*0.2));
        QCOMPARE(curItem->pos() + offset, QPointF(qRound(itemPos.x()), qRound(itemPos.y())));
    }

    delete canvas;
}

QQuickView *tst_QQuickPathView::createView()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setGeometry(0,0,240,320);

    return canvas;
}

/*
   Find an item with the specified objectName.  If index is supplied then the
   item must also evaluate the {index} expression equal to index
 */
template<typename T>
T *tst_QQuickPathView::findItem(QQuickItem *parent, const QString &objectName, int index)
{
    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->childItems().count() << "children";
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
        if (!item)
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

template<typename T>
QList<T*> tst_QQuickPathView::findItems(QQuickItem *parent, const QString &objectName)
{
    QList<T*> items;
    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->QQuickItem::children().count() << "children";
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
        if (!item)
            continue;
        //qDebug() << "try" << item;
        if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName))
            items.append(static_cast<T*>(item));
        items += findItems<T>(item, objectName);
    }

    return items;
}

void tst_QQuickPathView::missingPercent()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, testFileUrl("missingPercent.qml"));
    QDeclarativePath *obj = qobject_cast<QDeclarativePath*>(c.create());
    QVERIFY(obj);
    QCOMPARE(obj->attributeAt("_qfx_percent", 1.0), qreal(1.0));
    delete obj;
}


QTEST_MAIN(tst_QQuickPathView)

#include "tst_qquickpathview.moc"
