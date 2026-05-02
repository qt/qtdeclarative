// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQMLREGEXPFILTER_P_H
#define QQMLREGEXPFILTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qregularexpression.h>
#include <QtQmlModels/private/qqmlrolefilter_p.h>

QT_BEGIN_NAMESPACE

class QQmlSortFilterProxyModel;
class QQmlRegExpFilterPrivate;

class Q_QMLMODELS_EXPORT QQmlRegExpFilter : public QQmlRoleFilter
{
    Q_OBJECT
    Q_PROPERTY(QRegularExpression regExp READ regExp WRITE setRegExp NOTIFY regExpChanged FINAL)
    QML_NAMED_ELEMENT(RegExpFilter)
    QML_ADDED_IN_VERSION(6, 12)

public:
    explicit QQmlRegExpFilter(QObject *parent = nullptr);

    QRegularExpression regExp() const;
    void setRegExp(const QRegularExpression &regExp);

    bool filterAcceptsRowInternal(int row, const QModelIndex &sourceParent,
                                  const QQmlSortFilterProxyModel *proxyModel) const override;

Q_SIGNALS:
    void regExpChanged();

private:
    Q_DECLARE_PRIVATE(QQmlRegExpFilter)
};

class QQmlRegExpFilterPrivate : public QQmlRoleFilterPrivate
{
    Q_DECLARE_PUBLIC(QQmlRegExpFilter)

public:
    QRegularExpression regExp;
};

QT_END_NAMESPACE

#endif // QQMLREGEXPFILTER_P_H
