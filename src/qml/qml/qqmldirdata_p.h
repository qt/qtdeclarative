/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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
    QV4::CompiledData::Location importLocation(Blob *blob) const;

    template<typename Callback>
    bool processImports(Blob *blob, const Callback &callback) const
    {
        bool result = true;
        const auto range = m_imports.equal_range(blob);
        for (auto it = range.first; it != range.second; ++it) {
            // Do we need to resolve this import?
            if ((it->import->priority == 0) || (it->import->priority > it->priority)) {
                // This is the (current) best resolution for this import
                if (!callback(it->import))
                    result = false;
                it->import->priority = it->priority;
            }
        }
        return result;
    }

    void setPriority(Blob *, PendingImportPtr, int);

protected:
    void dataReceived(const SourceCodeData &) override;
    void initializeFromCachedUnit(const QQmlPrivate::CachedQmlUnit *) override;

private:
    struct PrioritizedImport {
        PendingImportPtr import;
        int priority = 0;
    };

    QString m_content;
    QMultiHash<Blob *, PrioritizedImport> m_imports;
};

QT_END_NAMESPACE

#endif // QQMLDIRDATA_P_H
