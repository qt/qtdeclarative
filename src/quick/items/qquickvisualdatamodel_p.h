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

#ifndef QQUICKVISUALDATAMODEL_P_H
#define QQUICKVISUALDATAMODEL_P_H

#include <private/qtquickglobal_p.h>
#include <private/qquicklistcompositor_p.h>
#include <private/qquickvisualitemmodel_p.h>

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qstringlist.h>

#include <private/qv8engine_p.h>
#include <private/qqmlglobal_p.h>

QT_BEGIN_HEADER

Q_DECLARE_METATYPE(QModelIndex)

QT_BEGIN_NAMESPACE

class QQuickChangeSet;
class QQmlComponent;
class QQuickPackage;
class QQmlV8Function;
class QQuickVisualDataGroup;
class QQuickVisualDataModelAttached;
class QQuickVisualDataModelPrivate;


class Q_QUICK_PRIVATE_EXPORT QQuickVisualDataModel : public QQuickVisualModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickVisualDataModel)

    Q_PROPERTY(QVariant model READ model WRITE setModel)
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate)
    Q_PROPERTY(QString filterOnGroup READ filterGroup WRITE setFilterGroup NOTIFY filterGroupChanged RESET resetFilterGroup)
    Q_PROPERTY(QQuickVisualDataGroup *items READ items CONSTANT)
    Q_PROPERTY(QQuickVisualDataGroup *persistedItems READ persistedItems CONSTANT)
    Q_PROPERTY(QQmlListProperty<QQuickVisualDataGroup> groups READ groups CONSTANT)
    Q_PROPERTY(QObject *parts READ parts CONSTANT)
    Q_PROPERTY(QVariant rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
    Q_CLASSINFO("DefaultProperty", "delegate")
    Q_INTERFACES(QQmlParserStatus)
public:
    QQuickVisualDataModel();
    QQuickVisualDataModel(QQmlContext *, QObject *parent=0);
    virtual ~QQuickVisualDataModel();

    void classBegin();
    void componentComplete();

    QVariant model() const;
    void setModel(const QVariant &);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *);

    QVariant rootIndex() const;
    void setRootIndex(const QVariant &root);

    Q_INVOKABLE QVariant modelIndex(int idx) const;
    Q_INVOKABLE QVariant parentModelIndex() const;

    int count() const;
    bool isValid() const { return delegate() != 0; }
    QQuickItem *item(int index, bool asynchronous=false);
    ReleaseFlags release(QQuickItem *item);
    void cancel(int index);
    virtual QString stringValue(int index, const QString &role);
    virtual void setWatchedRoles(QList<QByteArray> roles);

    int indexOf(QQuickItem *item, QObject *objectContext) const;

    QString filterGroup() const;
    void setFilterGroup(const QString &group);
    void resetFilterGroup();

    QQuickVisualDataGroup *items();
    QQuickVisualDataGroup *persistedItems();
    QQmlListProperty<QQuickVisualDataGroup> groups();
    QObject *parts();

    bool event(QEvent *);

    static QQuickVisualDataModelAttached *qmlAttachedProperties(QObject *obj);

Q_SIGNALS:
    void filterGroupChanged();
    void defaultGroupsChanged();
    void rootIndexChanged();

private Q_SLOTS:
    void _q_itemsChanged(int index, int count, const QVector<int> &roles);
    void _q_itemsInserted(int index, int count);
    void _q_itemsRemoved(int index, int count);
    void _q_itemsMoved(int from, int to, int count);
    void _q_modelReset();
    void _q_rowsInserted(const QModelIndex &,int,int);
    void _q_rowsAboutToBeRemoved(const QModelIndex &parent, int begin, int end);
    void _q_rowsRemoved(const QModelIndex &,int,int);
    void _q_rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int);
    void _q_dataChanged(const QModelIndex&,const QModelIndex&,const QVector<int> &);
    void _q_layoutChanged();

private:
    Q_DISABLE_COPY(QQuickVisualDataModel)
};

class QQuickVisualDataGroupPrivate;
class Q_AUTOTEST_EXPORT QQuickVisualDataGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool includeByDefault READ defaultInclude WRITE setDefaultInclude NOTIFY defaultIncludeChanged)
public:
    QQuickVisualDataGroup(QObject *parent = 0);
    QQuickVisualDataGroup(const QString &name, QQuickVisualDataModel *model, int compositorType, QObject *parent = 0);
    ~QQuickVisualDataGroup();

    QString name() const;
    void setName(const QString &name);

    int count() const;

    bool defaultInclude() const;
    void setDefaultInclude(bool include);

    Q_INVOKABLE QQmlV8Handle get(int index);

public Q_SLOTS:
    void insert(QQmlV8Function *);
    void create(QQmlV8Function *);
    void resolve(QQmlV8Function *);
    void remove(QQmlV8Function *);
    void addGroups(QQmlV8Function *);
    void removeGroups(QQmlV8Function *);
    void setGroups(QQmlV8Function *);
    void move(QQmlV8Function *);

Q_SIGNALS:
    void countChanged();
    void nameChanged();
    void defaultIncludeChanged();
    void changed(const QQmlV8Handle &removed, const QQmlV8Handle &inserted);
private:
    Q_DECLARE_PRIVATE(QQuickVisualDataGroup)
};

class QQuickVisualDataModelItem;
class QQuickVisualDataModelAttachedMetaObject;
class QQuickVisualDataModelAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickVisualDataModel *model READ model CONSTANT)
    Q_PROPERTY(QStringList groups READ groups WRITE setGroups NOTIFY groupsChanged)
    Q_PROPERTY(bool isUnresolved READ isUnresolved NOTIFY unresolvedChanged)
public:
    QQuickVisualDataModelAttached(QObject *parent);
    QQuickVisualDataModelAttached(QQuickVisualDataModelItem *cacheItem, QObject *parent);
    ~QQuickVisualDataModelAttached() {}

    void setCacheItem(QQuickVisualDataModelItem *item);

    QQuickVisualDataModel *model() const;

    QStringList groups() const;
    void setGroups(const QStringList &groups);

    bool isUnresolved() const;

    void emitChanges();

    void emitUnresolvedChanged() { emit unresolvedChanged(); }

Q_SIGNALS:
    void groupsChanged();
    void unresolvedChanged();

public:
    QQuickVisualDataModelItem *m_cacheItem;
    int m_previousGroups;
    int m_currentIndex[QQuickListCompositor::MaximumGroupCount];
    int m_previousIndex[QQuickListCompositor::MaximumGroupCount];

    friend class QQuickVisualDataModelAttachedMetaObject;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickVisualDataModel)
QML_DECLARE_TYPEINFO(QQuickVisualDataModel, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QQuickVisualDataGroup)

QT_END_HEADER

#endif // QQUICKVISUALDATAMODEL_P_H
