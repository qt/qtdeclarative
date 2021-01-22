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

#ifndef QQMLSCRIPTBLOB_P_H
#define QQMLSCRIPTBLOB_P_H

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

class QQmlScriptData;
class Q_AUTOTEST_EXPORT QQmlScriptBlob : public QQmlTypeLoader::Blob
{
private:
    friend class QQmlTypeLoader;

    QQmlScriptBlob(const QUrl &, QQmlTypeLoader *);

public:
    ~QQmlScriptBlob() override;

    struct ScriptReference
    {
        QV4::CompiledData::Location location;
        QString qualifier;
        QString nameSpace;
        QQmlRefPointer<QQmlScriptBlob> script;
    };

    QQmlRefPointer<QQmlScriptData> scriptData() const;

protected:
    void dataReceived(const SourceCodeData &) override;
    void initializeFromCachedUnit(const QV4::CompiledData::Unit *unit) override;
    void done() override;

    QString stringAt(int index) const override;

private:
    void scriptImported(const QQmlRefPointer<QQmlScriptBlob> &blob, const QV4::CompiledData::Location &location, const QString &qualifier, const QString &nameSpace) override;
    void initializeFromCompilationUnit(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit);

    QList<ScriptReference> m_scripts;
    QQmlRefPointer<QQmlScriptData> m_scriptData;
    const bool m_isModule;
};

QT_END_NAMESPACE

#endif // QQMLSCRIPTBLOB_P_H
