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
**/
#ifndef QQMLDOMFILEWRITER_P
#define QQMLDOMFILEWRITER_P

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

#include "qqmldom_global.h"
#include "qqmldomfunctionref_p.h"

#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

QMLDOM_EXPORT class FileWriter
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(FileWriter)
public:
    enum class Status { ShouldWrite, DidWrite, SkippedEqual, SkippedDueToFailure };

    FileWriter() = default;

    ~FileWriter()
    {
        if (!silentWarnings)
            for (QString w : warnings)
                qWarning() << w;
        if (shouldRemoveTempFile)
            tempFile.remove();
    }

    Status write(QString targetFile, function_ref<bool(QTextStream &)> write, int nBk = 2);

    bool shouldRemoveTempFile = false;
    bool silentWarnings = false;
    Status status = Status::SkippedDueToFailure;
    QString targetFile;
    QFile tempFile;
    QStringList newBkFiles;
    QStringList warnings;

private:
    Q_DISABLE_COPY_MOVE(FileWriter)
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
#endif
