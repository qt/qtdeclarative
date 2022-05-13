// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomfilewriter_p.h"
#include <QtCore/QRandomGenerator>
#include <QtCore/QScopeGuard>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

FileWriter::Status FileWriter::write(QString tFile, function_ref<bool(QTextStream &)> write,
                                     int nBk)
{
    if (shouldRemoveTempFile)
        tempFile.remove();
    tempFile.close();
    shouldRemoveTempFile = false;
    Q_ASSERT(status != Status::ShouldWrite);
    status = Status::ShouldWrite;
    targetFile = tFile;
    newBkFiles.clear();
    warnings.clear();

    int i = 0;
    const int maxAttempts = 20;
    for (; i < maxAttempts; ++i) {
        tempFile.setFileName(targetFile
                             + QString::number(QRandomGenerator::global()->generate(), 16).mid(0, 8)
                             + QStringLiteral(u".tmp"));
        if (tempFile.open(QIODevice::ReadWrite | QIODevice::NewOnly))
            break;
    }
    if (i == maxAttempts) {
        warnings.append(tr("Could not create temp file for %1").arg(targetFile));
        status = FileWriter::Status::SkippedDueToFailure;
        return status;
    }
    shouldRemoveTempFile = true;
    bool success = false;
    QTextStream inF(&tempFile);
    QT_TRY
    {
        auto cleanup = qScopeGuard([this, &inF, &success, nBk] {
            inF.flush();
            tempFile.flush();
            tempFile.close();
            if (success) {
                if (QFile::exists(targetFile)) {
                    // compareFiles
                    if (tempFile.open(QIODevice::ReadOnly)) {
                        auto closeTmpF = qScopeGuard([this] { tempFile.close(); });
                        QFile oldF(targetFile);
                        if (oldF.open(QIODevice::ReadOnly)) {
                            bool same = true;
                            while (!tempFile.atEnd() && !oldF.atEnd()) {
                                QByteArray l1 = tempFile.readLine();
                                QByteArray l2 = oldF.readLine();
                                if (l1 != l2)
                                    same = false;
                            }
                            if (tempFile.atEnd() && oldF.atEnd() && same) {
                                tempFile.remove();
                                shouldRemoveTempFile = false;
                                status = Status::SkippedEqual;
                                return;
                            }
                        }
                    }
                }
                // move to target
                int i = 0;
                const int maxAttempts = 10;
                for (; i < maxAttempts; ++i) {
                    if (QFile::exists(targetFile)) {
                        // make place for targetFile
                        QString bkFileName;
                        if (nBk < 1) {
                            QFile::remove(targetFile);
                        } else if (nBk == 1) {
                            QString bkFileName = targetFile + QStringLiteral(u"~");
                            QFile::remove(bkFileName);
                            QFile::rename(targetFile, bkFileName);
                        } else {
                            // f~ is the oldest, further backups at f1~ .. f<nBk>~
                            // keeping an empty place for the "next" backup
                            // f~ is never overwritten
                            int iBk = 0;
                            QString bkFileName = targetFile + QStringLiteral(u"~");
                            while (++iBk < nBk) {
                                if (QFile::exists(bkFileName))
                                    bkFileName = targetFile + QString::number(iBk)
                                            + QStringLiteral(u"~");
                            }
                            if (iBk == nBk)
                                QFile::remove(targetFile + QStringLiteral(u"1~"));
                            else
                                QFile::remove(targetFile + QString::number(++iBk)
                                              + QStringLiteral(u"~"));
                            QFile::remove(bkFileName);
                            QFile::rename(targetFile, bkFileName);
                        }
                        if (!bkFileName.isEmpty() && QFile::rename(targetFile, bkFileName))
                            newBkFiles.append(bkFileName);
                    }
                    if (tempFile.rename(targetFile)) {
                        status = Status::DidWrite;
                        shouldRemoveTempFile = false;
                        return;
                    }
                }
                warnings.append(
                        tr("Rename of file %1 to %2 failed").arg(tempFile.fileName(), targetFile));
                status = Status::SkippedDueToFailure;
            } else {
                warnings.append(tr("Error while writing"));
            }
        });
        success = write(inF);
    }
    QT_CATCH(...)
    {
        warnings.append(tr("Exception trying to write file %1").arg(targetFile));
        status = FileWriter::Status::SkippedDueToFailure;
    }
    if (status == Status::ShouldWrite)
        status = Status::SkippedDueToFailure;
    return status;
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#include "moc_qqmldomfilewriter_p.cpp"
