/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLTESTUTILS_P_H
#define QQMLTESTUTILS_P_H

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

#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtTest/QTest>

QT_BEGIN_NAMESPACE

/* Base class for tests with data that are located in a "data" subfolder. */

class QQmlDataTest : public QObject
{
    Q_OBJECT
public:
    QQmlDataTest(const char *qmlTestDataDir);
    virtual ~QQmlDataTest();

    QString testFile(const QString &fileName) const;
    inline QString testFile(const char *fileName) const
        { return testFile(QLatin1String(fileName)); }
    inline QUrl testFileUrl(const QString &fileName) const
        {
            const QString fn = testFile(fileName);
            return fn.startsWith(QLatin1Char(':'))
                ? QUrl(QLatin1String("qrc") + fn)
                : QUrl::fromLocalFile(fn);
        }
    inline QUrl testFileUrl(const char *fileName) const
        { return testFileUrl(QLatin1String(fileName)); }

    inline QString dataDirectory() const { return m_dataDirectory; }
    inline QUrl dataDirectoryUrl() const { return m_dataDirectoryUrl; }
    inline QString directory() const  { return m_directory; }

    static inline QQmlDataTest *instance() { return m_instance; }

    bool canImportModule(const QString &importTestQmlSource) const;

public slots:
    virtual void initTestCase();

private:
    static QQmlDataTest *m_instance;

    // The directory in which to search for the "data" directory.
    const char *m_qmlTestDataDir = nullptr;
    // The path to the "data" directory, if found.
    const QString m_dataDirectory;
    const QUrl m_dataDirectoryUrl;
    QString m_directory;
};

class QQmlTestMessageHandler
{
    Q_DISABLE_COPY(QQmlTestMessageHandler)
public:
    QQmlTestMessageHandler();
    ~QQmlTestMessageHandler();

    const QStringList &messages() const { return m_messages; }
    const QString messageString() const { return m_messages.join(QLatin1Char('\n')); }

    void clear() { m_messages.clear(); }

    void setIncludeCategoriesEnabled(bool enabled) { m_includeCategories = enabled; }

private:
    static void messageHandler(QtMsgType, const QMessageLogContext &context, const QString &message);

    static QQmlTestMessageHandler *m_instance;
    QStringList m_messages;
    QtMessageHandler m_oldHandler;
    bool m_includeCategories;
};

QT_END_NAMESPACE

#endif // QQMLTESTUTILS_P_H
