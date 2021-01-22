/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
**
**
****************************************************************************/

#ifndef QQMLDIRDATA_P_H
#define QQMLDIRDATA_P_H

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

#include <private/qqmltypeloader_p.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQmlQmldirData : public QQmlTypeLoader::Blob
{
private:
    friend class QQmlTypeLoader;

    QQmlQmldirData(const QUrl &, QQmlTypeLoader *);

public:
    const QString &content() const;

    PendingImportPtr import(QQmlTypeLoader::Blob *) const;
    void setImport(QQmlTypeLoader::Blob *, PendingImportPtr);

    int priority(QQmlTypeLoader::Blob *) const;
    void setPriority(QQmlTypeLoader::Blob *, int);

protected:
    void dataReceived(const SourceCodeData &) override;
    void initializeFromCachedUnit(const QV4::CompiledData::Unit *) override;

private:
    QString m_content;
    QHash<QQmlTypeLoader::Blob *, QQmlTypeLoader::Blob::PendingImportPtr> m_imports;
    QHash<QQmlTypeLoader::Blob *, int> m_priorities;
};

QT_END_NAMESPACE

#endif // QQMLDIRDATA_P_H
