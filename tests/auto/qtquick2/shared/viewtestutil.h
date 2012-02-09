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

#ifndef QQUICKVIEWTESTUTIL_H
#define QQUICKVIEWTESTUTIL_H

#include <QtQuick/QQuickItem>
#include <QtDeclarative/QDeclarativeExpression>
#include <QtDeclarative/private/qlistmodelinterface_p.h>
#include <QtCore/QAbstractListModel>

QT_FORWARD_DECLARE_CLASS(QQuickView)

namespace QQuickViewTestUtil
{
    QQuickView *createView();

    void flick(QQuickView *canvas, const QPoint &from, const QPoint &to, int duration);

    QList<int> adjustIndexesForAddDisplaced(const QList<int> &indexes, int index, int count);
    QList<int> adjustIndexesForMove(const QList<int> &indexes, int from, int to, int count);
    QList<int> adjustIndexesForRemoveDisplaced(const QList<int> &indexes, int index, int count);

    struct ListChange {
        enum { Inserted, Removed, Moved, SetCurrent, SetContentY } type;
        int index;
        int count;
        int to;     // Move
        qreal pos;  // setContentY

        static ListChange insert(int index, int count = 1) { ListChange c = { Inserted, index, count, -1, 0.0 }; return c; }
        static ListChange remove(int index, int count = 1) { ListChange c = { Removed, index, count, -1, 0.0 }; return c; }
        static ListChange move(int index, int to, int count) { ListChange c = { Moved, index, count, to, 0.0 }; return c; }
        static ListChange setCurrent(int index) { ListChange c = { SetCurrent, index, -1, -1, 0.0 }; return c; }
        static ListChange setContentY(qreal pos) { ListChange c = { SetContentY, -1, -1, -1, pos }; return c; }
    };

    class QmlListModel : public QListModelInterface
    {
        Q_OBJECT
    public:
        QmlListModel(QObject *parent = 0);
        ~QmlListModel();

        enum Roles { Name, Number };

        QString name(int index) const;
        QString number(int index) const;

        int count() const;

        QList<int> roles() const;
        QString toString(int role) const;

        QVariant data(int index, int role) const;
        QHash<int, QVariant> data(int index, const QList<int> &roles) const;

        Q_INVOKABLE void addItem(const QString &name, const QString &number);
        void insertItem(int index, const QString &name, const QString &number);
        void insertItems(int index, const QList<QPair<QString, QString> > &items);

        void removeItem(int index);
        void removeItems(int index, int count);

        void moveItem(int from, int to);
        void moveItems(int from, int to, int count);

        void modifyItem(int index, const QString &name, const QString &number);

        void clear();

        void matchAgainst(const QList<QPair<QString, QString> > &other, const QString &error1, const QString &error2);

    private:
        QList<QPair<QString,QString> > list;
    };

    class QaimModel : public QAbstractListModel
    {
        Q_OBJECT
    public:
        enum Roles { Name = Qt::UserRole+1, Number = Qt::UserRole+2 };

        QaimModel(QObject *parent=0);

        int rowCount(const QModelIndex &parent=QModelIndex()) const;
        QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const;

        int count() const;
        QString name(int index) const;
        QString number(int index) const;

        Q_INVOKABLE void addItem(const QString &name, const QString &number);
        void addItems(const QList<QPair<QString, QString> > &items);
        void insertItem(int index, const QString &name, const QString &number);
        void insertItems(int index, const QList<QPair<QString, QString> > &items);

        void removeItem(int index);
        void removeItems(int index, int count);

        void moveItem(int from, int to);
        void moveItems(int from, int to, int count);

        void modifyItem(int idx, const QString &name, const QString &number);

        void clear();
        void reset();

        void matchAgainst(const QList<QPair<QString, QString> > &other, const QString &error1, const QString &error2);

    private:
        QList<QPair<QString,QString> > list;
    };

    class ListRange
    {
    public:
        ListRange();
        ListRange(const ListRange &other);
        ListRange(int start, int end);

        ~ListRange();

        ListRange operator+(const ListRange &other) const;
        bool operator==(const ListRange &other) const;
        bool operator!=(const ListRange &other) const;

        bool isValid() const;
        int count() const;

        QList<QPair<QString,QString> > getModelDataValues(const QmlListModel &model);
        QList<QPair<QString,QString> > getModelDataValues(const QaimModel &model);

        QList<int> indexes;
        bool valid;
    };
}

Q_DECLARE_METATYPE(QList<QQuickViewTestUtil::ListChange>)
Q_DECLARE_METATYPE(QQuickViewTestUtil::ListRange)

#endif // QQUICKVIEWTESTUTIL_H
