/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKVISUALADAPTORMODEL_P_H
#define QQUICKVISUALADAPTORMODEL_P_H

#include <QtCore/qabstractitemmodel.h>

#include "private/qquicklistaccessor_p.h"

#include <private/qqmlguard_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQmlEngine;

class QQuickVisualDataModel;
class QQuickVisualDataModelItem;
class QQuickVisualDataModelItemMetaType;

class QQuickVisualAdaptorModel : public QQmlGuard<QObject>
{
public:
    class Accessors
    {
    public:
        inline Accessors() {}
        virtual ~Accessors();
        virtual int count(const QQuickVisualAdaptorModel &) const { return 0; }
        virtual void cleanup(QQuickVisualAdaptorModel &, QQuickVisualDataModel * = 0) const {}

        virtual QVariant value(const QQuickVisualAdaptorModel &, int, const QString &) const {
            return QVariant(); }

        virtual QQuickVisualDataModelItem *createItem(
                QQuickVisualAdaptorModel &,
                QQuickVisualDataModelItemMetaType *,
                QQmlEngine *,
                int) const { return 0; }

        virtual bool notify(
                const QQuickVisualAdaptorModel &,
                const QList<QQuickVisualDataModelItem *> &,
                int,
                int,
                const QVector<int> &) const { return false; }
        virtual void replaceWatchedRoles(
                QQuickVisualAdaptorModel &,
                const QList<QByteArray> &,
                const QList<QByteArray> &) const {}
        virtual QVariant parentModelIndex(const QQuickVisualAdaptorModel &) const {
            return QVariant(); }
        virtual QVariant modelIndex(const QQuickVisualAdaptorModel &, int) const {
            return QVariant(); }
        virtual bool canFetchMore(const QQuickVisualAdaptorModel &) const { return false; }
        virtual void fetchMore(QQuickVisualAdaptorModel &) const {}
    };

    const Accessors *accessors;
    QPersistentModelIndex rootIndex;
    QQuickListAccessor list;

    QQuickVisualAdaptorModel();
    ~QQuickVisualAdaptorModel();

    inline QVariant model() const { return list.list(); }
    void setModel(const QVariant &variant, QQuickVisualDataModel *vdm, QQmlEngine *engine);
    void invalidateModel(QQuickVisualDataModel *vdm);

    bool isValid() const;

    inline QAbstractItemModel *aim() { return static_cast<QAbstractItemModel *>(object()); }
    inline const QAbstractItemModel *aim() const { return static_cast<const QAbstractItemModel *>(object()); }

    inline int count() const { return qMax(0, accessors->count(*this)); }
    inline QVariant value(int index, const QString &role) const {
        return accessors->value(*this, index, role); }
    inline QQuickVisualDataModelItem *createItem(QQuickVisualDataModelItemMetaType *metaType, QQmlEngine *engine, int index) {
        return accessors->createItem(*this, metaType, engine, index); }
    inline bool hasProxyObject() const {
        return list.type() == QQuickListAccessor::Instance || list.type() == QQuickListAccessor::ListProperty; }

    inline bool notify(
            const QList<QQuickVisualDataModelItem *> &items,
            int index,
            int count,
            const QVector<int> &roles) const {
        return accessors->notify(*this, items, index, count, roles); }
    inline void replaceWatchedRoles(
            const QList<QByteArray> &oldRoles, const QList<QByteArray> &newRoles) {
        accessors->replaceWatchedRoles(*this, oldRoles, newRoles); }

    inline QVariant modelIndex(int index) const { return accessors->modelIndex(*this, index); }
    inline QVariant parentModelIndex() const { return accessors->parentModelIndex(*this); }
    inline bool canFetchMore() const { return accessors->canFetchMore(*this); }
    inline void fetchMore() { return accessors->fetchMore(*this); }

protected:
    void objectDestroyed(QObject *);
};

class QQuickVisualAdaptorModelProxyInterface
{
public:
    virtual ~QQuickVisualAdaptorModelProxyInterface() {}

    virtual QObject *proxiedObject() = 0;
};

#define QQuickVisualAdaptorModelProxyInterface_iid "org.qt-project.Qt.QQuickVisualAdaptorModelProxyInterface"

Q_DECLARE_INTERFACE(QQuickVisualAdaptorModelProxyInterface, QQuickVisualAdaptorModelProxyInterface_iid)

QT_END_NAMESPACE

#endif
