/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQMLJSTYPEREADER_P_H
#define QQMLJSTYPEREADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljsscope_p.h"
#include "qqmljsimporter_p.h"
#include "qqmljsresourcefilemapper_p.h"

#include <QtQml/private/qqmljsastfwd_p.h>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>

#include <QtCore/qpair.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

class QQmlJSTypeReader
{
public:
    QQmlJSTypeReader(QQmlJSImporter *importer, const QString &file,
                     const QStringList &qmltypesFiles = QStringList())
        : m_importer(importer)
        , m_file(file)
        , m_qmltypesFiles(qmltypesFiles)
    {}

    bool operator()(const QSharedPointer<QQmlJSScope> &scope);
    QList<QQmlJS::DiagnosticMessage> errors() const { return m_errors; }

private:
    QQmlJSImporter *m_importer;
    QString m_file;
    QStringList m_qmltypesFiles;
    QList<QQmlJS::DiagnosticMessage> m_errors;
};

QT_END_NAMESPACE

#endif // QQMLJSTYPEREADER_P_H
