// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltyperegistrarutils_p.h"

#include "qanystringviewutils_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qqmltyperegistrarconstants_p.h"
#include "qmetatypesjsonprocessor_p.h"

#include <QtCore/qcborarray.h>
#include <QtCore/qcbormap.h>
#include <QtCore/qcborvalue.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/qtextstream.h>

using namespace Qt::Literals::StringLiterals;

QT_BEGIN_NAMESPACE

QTypeRevision handleInMinorVersion(QTypeRevision revision, int majorVersion)
{
    if (!revision.hasMajorVersion() && revision.hasMinorVersion()) {
        // this version has been obtained by QML_{ADDED,REMOVED}_IN_MINOR_VERSION
        revision = QTypeRevision::fromVersion(majorVersion, revision.minorVersion());
    }
    return revision;
}

QAnyStringView interfaceName(const Interface &iface)
{
    return iface.className;
}

static QDebug message(QDebug base, QAnyStringView message, QAnyStringView fileName, int lineNumber)
{
    // Formatted such that it becomes a link in QtCreator (even with "::" ending)
    const QString lineString = lineNumber ? QString::number(lineNumber) : QString();
    return (base.noquote().nospace()
            << message << ": " << fileName << ":" << lineString << ":").space();
}

QDebug warning(QAnyStringView fileName, int lineNumber)
{
    return message(qWarning(), "Warning", fileName, lineNumber);
}

QDebug warning(const MetaType &classDef)
{
    const QAnyStringView file = classDef.inputFile();
    int lineNo = classDef.lineNumber();
    if (!file.isEmpty())
        return warning(file, lineNo);

    return warning(classDef.qualifiedClassName());
}

QDebug error(QAnyStringView fileName, int lineNumber)
{
    return message(qCritical(), "Error", fileName, lineNumber);
}

/*!
    \internal
    \a pathToList points to a file listing all qt.parts.conf files
    In any given directory, there might be more than one qt.parts.conf file (especially on Winodws).
    We need to merge all import paths for a a given folder (but want to avoid duplicate entries).
 */
int mergeQtConfFiles(const QString &pathToList)
{
    QFile listFile(pathToList);
    if (!listFile.open(QFile::ReadOnly | QFile::Text))
        return EXIT_FAILURE;
    QMultiHash<QString, QString> directoryToNecessaryImports;
    while (!listFile.atEnd()) {
        QByteArray partFilePath = listFile.readLine().trimmed();
        QString directoryPath = QFileInfo(QString::fromUtf8(partFilePath)).absolutePath();
        QDirIterator dirIt(directoryPath, { "*_qt.part.conf"_L1 }, QDir::Filter::Files );
        while (dirIt.hasNext()) {
            QFile partialFile(dirIt.next());
            if (!partialFile.open(QFile::ReadOnly | QFile::Text)) {
                qDebug() << "could not open" << partialFile.fileName();
                return EXIT_FAILURE;
            }
            while (!partialFile.atEnd()) {
                QByteArray import = partialFile.readLine().trimmed();
                directoryToNecessaryImports.insert(directoryPath, QString::fromUtf8(import));
            }
        }
    }
    for (const QString &directoryPath: directoryToNecessaryImports.keys()) {
        QFile consolidatedQtConfFile(directoryPath + QDir::separator() + u"qt.conf");
        if (!consolidatedQtConfFile.open(QFile::WriteOnly | QFile::Text)) {
            qDebug() << "could not open" << consolidatedQtConfFile.fileName();
            return EXIT_FAILURE;
        }
        QTextStream out(&consolidatedQtConfFile);
        QStringList allIncludes = directoryToNecessaryImports.values(directoryPath);
        allIncludes.removeDuplicates();
        out << "[Config]\nMergeQtConf = true\n"
            << "[Paths]\nQmlImports = "
            << allIncludes.join(u",") << Qt::endl;
    }
    QFile outfile(pathToList + u".done");
    if (!outfile.open(QFile::WriteOnly | QFile::Text))
        return EXIT_FAILURE;
    outfile.write(QByteArray::number(QDateTime::currentSecsSinceEpoch()));
    return EXIT_SUCCESS;
}

QT_END_NAMESPACE
