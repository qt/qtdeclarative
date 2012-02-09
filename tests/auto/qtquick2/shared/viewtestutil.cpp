/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "viewtestutil.h"

#include <QtQuick/QQuickView>

#include <QtTest/QTest>

template<typename T>
static void qdeclarativemodelviewstestutil_move(int from, int to, int n, T *items)
{
    if (from > to) {
        // Only move forwards - flip if backwards moving
        int tfrom = from;
        int tto = to;
        from = tto;
        to = tto+n;
        n = tfrom-tto;
    }

    T replaced;
    int i=0;
    typename T::ConstIterator it=items->begin(); it += from+n;
    for (; i<to-from; ++i,++it)
        replaced.append(*it);
    i=0;
    it=items->begin(); it += from;
    for (; i<n; ++i,++it)
        replaced.append(*it);
    typename T::ConstIterator f=replaced.begin();
    typename T::Iterator t=items->begin(); t += from;
    for (; f != replaced.end(); ++f, ++t)
        *t = *f;
}

QQuickView *QQuickViewTestUtil::createView()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setGeometry(0,0,240,320);

    return canvas;
}

void QQuickViewTestUtil::flick(QQuickView *canvas, const QPoint &from, const QPoint &to, int duration)
{
    const int pointCount = 5;
    QPoint diff = to - from;

    // send press, five equally spaced moves, and release.
    QTest::mousePress(canvas, Qt::LeftButton, 0, from);

    for (int i = 0; i < pointCount; ++i) {
        QMouseEvent mv(QEvent::MouseMove, from + (i+1)*diff/pointCount, Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
        QGuiApplication::sendEvent(canvas, &mv);
        QTest::qWait(duration/pointCount);
        QCoreApplication::processEvents();
    }

    QTest::mouseRelease(canvas, Qt::LeftButton, 0, to);
    QTest::qWait(50);
}

QList<int> QQuickViewTestUtil::adjustIndexesForAddDisplaced(const QList<int> &indexes, int index, int count)
{
    QList<int> result;
    for (int i=0; i<indexes.count(); i++) {
        int num = indexes[i];
        if (num >= index) {
            num += count;
        }
        result << num;
    }
    return result;
}

QList<int> QQuickViewTestUtil::adjustIndexesForMove(const QList<int> &indexes, int from, int to, int count)
{
    QList<int> result;
    for (int i=0; i<indexes.count(); i++) {
        int num = indexes[i];
        if (from < to) {
            if (num >= from && num < from + count)
                num += (to - from); // target
            else if (num >= from && num < to + count)
                num -= count;   // displaced
        } else if (from > to) {
            if (num >= from && num < from + count)
                num -= (from - to);  // target
            else if (num >= to && num < from + count)
                num += count;   // displaced
        }
        result << num;
    }
    return result;
}

QList<int> QQuickViewTestUtil::adjustIndexesForRemoveDisplaced(const QList<int> &indexes, int index, int count)
{
    QList<int> result;
    for (int i=0; i<indexes.count(); i++) {
        int num = indexes[i];
        if (num >= index)
            num -= count;
        result << num;
    }
    return result;
}


QQuickViewTestUtil::QmlListModel::QmlListModel(QObject *parent)
    : QListModelInterface(parent)
{
}

QQuickViewTestUtil::QmlListModel::~QmlListModel()
{
}

QString QQuickViewTestUtil::QmlListModel::name(int index) const
{
    return list.at(index).first;
}

QString QQuickViewTestUtil::QmlListModel::number(int index) const
{
    return list.at(index).second;
}

int QQuickViewTestUtil::QmlListModel::count() const
{
    return list.count();
}

QList<int> QQuickViewTestUtil::QmlListModel::roles() const
{
    return QList<int>() << Name << Number;
}

QString QQuickViewTestUtil::QmlListModel::toString(int role) const
{
    switch (role) {
    case Name:
        return "name";
    case Number:
        return "number";
    default:
        return "";
    }
}

QVariant QQuickViewTestUtil::QmlListModel::data(int index, int role) const
{
    if (role==0)
        return list.at(index).first;
    if (role==1)
        return list.at(index).second;
    return QVariant();
}

QHash<int, QVariant> QQuickViewTestUtil::QmlListModel::data(int index, const QList<int> &roles) const
{
    QHash<int,QVariant> returnHash;

    for (int i = 0; i < roles.size(); ++i) {
        int role = roles.at(i);
        QVariant info;
        switch (role) {
        case Name:
            info = list.at(index).first;
            break;
        case Number:
            info = list.at(index).second;
            break;
        default:
            break;
        }
        returnHash.insert(role, info);
    }
    return returnHash;
}

void QQuickViewTestUtil::QmlListModel::addItem(const QString &name, const QString &number)
{
    list.append(QPair<QString,QString>(name, number));
    emit itemsInserted(list.count()-1, 1);
}

void QQuickViewTestUtil::QmlListModel::insertItem(int index, const QString &name, const QString &number)
{
    list.insert(index, QPair<QString,QString>(name, number));
    emit itemsInserted(index, 1);
}

void QQuickViewTestUtil::QmlListModel::insertItems(int index, const QList<QPair<QString, QString> > &items)
{
    for (int i=0; i<items.count(); i++)
        list.insert(index + i, QPair<QString,QString>(items[i].first, items[i].second));
    emit itemsInserted(index, items.count());
}

void QQuickViewTestUtil::QmlListModel::removeItem(int index)
{
    list.removeAt(index);
    emit itemsRemoved(index, 1);
}

void QQuickViewTestUtil::QmlListModel::removeItems(int index, int count)
{
    int c = count;
    while (c--)
        list.removeAt(index);
    emit itemsRemoved(index, count);
}

void QQuickViewTestUtil::QmlListModel::moveItem(int from, int to)
{
    list.move(from, to);
    emit itemsMoved(from, to, 1);
}

void QQuickViewTestUtil::QmlListModel::moveItems(int from, int to, int count)
{
    qdeclarativemodelviewstestutil_move(from, to, count, &list);
    emit itemsMoved(from, to, count);
}

void QQuickViewTestUtil::QmlListModel::modifyItem(int index, const QString &name, const QString &number)
{
    list[index] = QPair<QString,QString>(name, number);
    emit itemsChanged(index, 1, roles());
}

void QQuickViewTestUtil::QmlListModel::clear() {
    int count = list.count();
    list.clear();
    emit itemsRemoved(0, count);
}

void QQuickViewTestUtil::QmlListModel::matchAgainst(const QList<QPair<QString, QString> > &other, const QString &error1, const QString &error2) {
    for (int i=0; i<other.count(); i++) {
        QVERIFY2(list.contains(other[i]),
                 QTest::toString(other[i].first + " " + other[i].second + " " + error1));
    }
    for (int i=0; i<list.count(); i++) {
        QVERIFY2(other.contains(list[i]),
                 QTest::toString(list[i].first + " " + list[i].second + " " + error2));
    }
}


QQuickViewTestUtil::QaimModel::QaimModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[Name] = "name";
    roles[Number] = "number";
    setRoleNames(roles);
}

int QQuickViewTestUtil::QaimModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return list.count();
}

QVariant QQuickViewTestUtil::QaimModel::data(const QModelIndex &index, int role) const
{
    QVariant rv;
    if (role == Name)
        rv = list.at(index.row()).first;
    else if (role == Number)
        rv = list.at(index.row()).second;

    return rv;
}

int QQuickViewTestUtil::QaimModel::count() const
{
    return rowCount();
}

QString QQuickViewTestUtil::QaimModel::name(int index) const
{
    return list.at(index).first;
}

QString QQuickViewTestUtil::QaimModel::number(int index) const
{
    return list.at(index).second;
}

void QQuickViewTestUtil::QaimModel::addItem(const QString &name, const QString &number)
{
    emit beginInsertRows(QModelIndex(), list.count(), list.count());
    list.append(QPair<QString,QString>(name, number));
    emit endInsertRows();
}

void QQuickViewTestUtil::QaimModel::addItems(const QList<QPair<QString, QString> > &items)
{
    emit beginInsertRows(QModelIndex(), list.count(), list.count()+items.count()-1);
    for (int i=0; i<items.count(); i++)
        list.append(QPair<QString,QString>(items[i].first, items[i].second));
    emit endInsertRows();
}

void QQuickViewTestUtil::QaimModel::insertItem(int index, const QString &name, const QString &number)
{
    emit beginInsertRows(QModelIndex(), index, index);
    list.insert(index, QPair<QString,QString>(name, number));
    emit endInsertRows();
}

void QQuickViewTestUtil::QaimModel::insertItems(int index, const QList<QPair<QString, QString> > &items)
{
    emit beginInsertRows(QModelIndex(), index, index+items.count()-1);
    for (int i=0; i<items.count(); i++)
        list.insert(index + i, QPair<QString,QString>(items[i].first, items[i].second));
    emit endInsertRows();
}

void QQuickViewTestUtil::QaimModel::removeItem(int index)
{
    emit beginRemoveRows(QModelIndex(), index, index);
    list.removeAt(index);
    emit endRemoveRows();
}

void QQuickViewTestUtil::QaimModel::removeItems(int index, int count)
{
    emit beginRemoveRows(QModelIndex(), index, index+count-1);
    while (count--)
        list.removeAt(index);
    emit endRemoveRows();
}

void QQuickViewTestUtil::QaimModel::moveItem(int from, int to)
{
    emit beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
    list.move(from, to);
    emit endMoveRows();
}

void QQuickViewTestUtil::QaimModel::moveItems(int from, int to, int count)
{
    emit beginMoveRows(QModelIndex(), from, from+count-1, QModelIndex(), to > from ? to+count : to);
    qdeclarativemodelviewstestutil_move(from, to, count, &list);
    emit endMoveRows();
}

void QQuickViewTestUtil::QaimModel::modifyItem(int idx, const QString &name, const QString &number)
{
    list[idx] = QPair<QString,QString>(name, number);
    emit dataChanged(index(idx,0), index(idx,0));
}

void QQuickViewTestUtil::QaimModel::clear()
{
    int count = list.count();
    emit beginRemoveRows(QModelIndex(), 0, count-1);
    list.clear();
    emit endRemoveRows();
}

void QQuickViewTestUtil::QaimModel::reset()
{
    emit beginResetModel();
    emit endResetModel();
}

void QQuickViewTestUtil::QaimModel::matchAgainst(const QList<QPair<QString, QString> > &other, const QString &error1, const QString &error2) {
    for (int i=0; i<other.count(); i++) {
        QVERIFY2(list.contains(other[i]),
                 QTest::toString(other[i].first + " " + other[i].second + " " + error1));
    }
    for (int i=0; i<list.count(); i++) {
        QVERIFY2(other.contains(list[i]),
                 QTest::toString(list[i].first + " " + list[i].second + " " + error2));
    }
}



QQuickViewTestUtil::ListRange::ListRange()
    : valid(false)
{
}

QQuickViewTestUtil::ListRange::ListRange(const ListRange &other)
    : valid(other.valid)
{
    indexes = other.indexes;
}

QQuickViewTestUtil::ListRange::ListRange(int start, int end)
    : valid(true)
{
    for (int i=start; i<=end; i++)
        indexes << i;
}

QQuickViewTestUtil::ListRange::~ListRange()
{
}

QQuickViewTestUtil::ListRange QQuickViewTestUtil::ListRange::operator+(const ListRange &other) const
{
    if (other == *this)
        return *this;
    ListRange a(*this);
    a.indexes.append(other.indexes);
    return a;
}

bool QQuickViewTestUtil::ListRange::operator==(const ListRange &other) const
{
    return indexes.toSet() == other.indexes.toSet();
}

bool QQuickViewTestUtil::ListRange::operator!=(const ListRange &other) const
{
    return !(*this == other);
}

bool QQuickViewTestUtil::ListRange::isValid() const
{
    return valid;
}

int QQuickViewTestUtil::ListRange::count() const
{
    return indexes.count();
}

QList<QPair<QString,QString> > QQuickViewTestUtil::ListRange::getModelDataValues(const QmlListModel &model)
{
    QList<QPair<QString,QString> > data;
    if (!valid)
        return data;
    for (int i=0; i<indexes.count(); i++)
        data.append(qMakePair(model.name(indexes[i]), model.number(indexes[i])));
    return data;
}

QList<QPair<QString,QString> > QQuickViewTestUtil::ListRange::getModelDataValues(const QaimModel &model)
{
    QList<QPair<QString,QString> > data;
    if (!valid)
        return data;
    for (int i=0; i<indexes.count(); i++)
        data.append(qMakePair(model.name(indexes[i]), model.number(indexes[i])));
    return data;
}

