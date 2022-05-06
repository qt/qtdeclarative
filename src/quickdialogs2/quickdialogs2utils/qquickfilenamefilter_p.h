/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQUICKFILENAMEFILTER_P_H
#define QQUICKFILENAMEFILTER_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qstringlist.h>
#include <QtGui/qpa/qplatformdialoghelper.h>

#include "qtquickdialogs2utilsglobal_p.h"

QT_BEGIN_NAMESPACE

class Q_QUICKDIALOGS2UTILS_PRIVATE_EXPORT QQuickFileNameFilter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged FINAL)
    Q_PROPERTY(QStringList extensions READ extensions NOTIFY extensionsChanged FINAL)
    Q_PROPERTY(QStringList globs READ globs NOTIFY globsChanged FINAL)

public:
    explicit QQuickFileNameFilter(QObject *parent = nullptr);

    int index() const;
    void setIndex(int index);

    QString name() const;
    QStringList extensions() const;
    QStringList globs() const;

    QSharedPointer<QFileDialogOptions> options() const;
    void setOptions(const QSharedPointer<QFileDialogOptions> &options);

    void update(const QString &filter);

Q_SIGNALS:
    void indexChanged(int index);
    void nameChanged(const QString &name);
    void extensionsChanged(const QStringList &extensions);
    void globsChanged(const QStringList &globs);

private:
    QStringList nameFilters() const;
    QString nameFilter(int index) const;

    int m_index;
    QString m_name;
    QStringList m_extensions;
    QStringList m_globs;
    QSharedPointer<QFileDialogOptions> m_options;
};

QT_END_NAMESPACE

#endif // QQUICKFILENAMEFILTER_P_H
