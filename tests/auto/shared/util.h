/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVETESTUTILS_H
#define QDECLARATIVETESTUTILS_H

#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtCore/QCoreApplication>
#include <QtTest/QTest>

QT_FORWARD_DECLARE_CLASS(QDeclarativeComponent)
QT_FORWARD_DECLARE_CLASS(QDeclarativeEngine)

/* Base class for tests with data that are located in a "data" subfolder. */

class QDeclarativeDataTest : public QObject
{
    Q_OBJECT
public:
    QDeclarativeDataTest();
    virtual ~QDeclarativeDataTest();

    QString testFile(const QString &fileName) const;
    inline QString testFile(const char *fileName) const
        { return testFile(QLatin1String(fileName)); }
    inline QUrl testFileUrl(const QString &fileName) const
        { return QUrl::fromLocalFile(testFile(fileName)); }
    inline QUrl testFileUrl(const char *fileName) const
        { return testFileUrl(QLatin1String(fileName)); }

    inline QString dataDirectory() const { return m_dataDirectory; }
    inline QUrl dataDirectoryUrl() const { return m_dataDirectoryUrl; }
    inline QString directory() const  { return m_directory; }

    static inline QDeclarativeDataTest *instance() { return m_instance; }

    static QByteArray msgComponentError(const QDeclarativeComponent &,
                                        const QDeclarativeEngine *engine = 0);

public slots:
    virtual void initTestCase();

private:
    static QDeclarativeDataTest *m_instance;

    const QString m_dataDirectory;
    const QUrl m_dataDirectoryUrl;
    QString m_directory;
};

#endif // QDECLARATIVETESTUTILS_H
