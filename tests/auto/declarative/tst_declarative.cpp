/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest>
#include <QtQml>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>

class tst_Declarative : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testFiles();
    void testFunctions();
    void testFunctions_data();
    void testSignalHandlers();
    void testSignalHandlers_data();

private:
    QMap<QString, QString> files;
};

class BaseValidator : public QQmlJS::AST::Visitor
{
public:
    QString errors() const { return m_errors.join(", "); }

    bool validate(const QString& filePath)
    {
        m_errors.clear();
        m_fileName = QFileInfo(filePath).fileName();

        QFile file(filePath);
        if (!file.open(QFile::ReadOnly)) {
            m_errors += QString("%1: failed to open (%2)").arg(m_fileName, file.errorString());
            return false;
        }

        QQmlJS::Engine engine;
        QQmlJS::Lexer lexer(&engine);
        lexer.setCode(QString::fromUtf8(file.readAll()), /*line = */ 1);

        QQmlJS::Parser parser(&engine);
        if (!parser.parse()) {
            foreach (const QQmlJS::DiagnosticMessage &msg, parser.diagnosticMessages())
                m_errors += QString("%s:%d : %s").arg(m_fileName).arg(msg.loc.startLine).arg(msg.message);
            return false;
        }

        QQmlJS::AST::UiProgram* ast = parser.ast();
        ast->accept(this);
        return m_errors.isEmpty();
    }

protected:
    void addError(const QString& error, QQmlJS::AST::Node *node)
    {
        m_errors += QString("%1:%2 : %3").arg(m_fileName).arg(node->firstSourceLocation().startLine).arg(error);
    }

private:
    QString m_fileName;
    QStringList m_errors;
};

static QMap<QString, QString> listQmlFiles(const QDir &dir)
{
    QMap<QString, QString> files;
    foreach (const QFileInfo &entry, dir.entryInfoList(QStringList() << "*.qml" << "*.js", QDir::Files))
        files.insert(entry.baseName(), entry.absoluteFilePath());
    return files;
}

void tst_Declarative::initTestCase()
{
    QQmlEngine engine;
    foreach (const QString &path, engine.importPathList()) {
        files.unite(listQmlFiles(QDir(path + "/QtQuick/Controls.2")));
        files.unite(listQmlFiles(QDir(path + "/QtQuick/Extras.2")));
    }
}

void tst_Declarative::testFiles()
{
    QMap<QString, QString>::const_iterator it;
    for (it = files.begin(); it != files.end(); ++it) {
        if (QFileInfo(it.value()).suffix() == QString("js"))
            QFAIL(qPrintable(it.value() +  ": JS files are not allowed"));
    }
}

class FunctionValidator : public BaseValidator
{
protected:
    virtual bool visit(QQmlJS::AST::FunctionDeclaration *node)
    {
        addError("function declarations are not allowed", node);
        return true;
    }
};

void tst_Declarative::testFunctions()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    FunctionValidator validator;
    if (!validator.validate(filePath))
        QFAIL(qPrintable(validator.errors()));
}

void tst_Declarative::testFunctions_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = files.begin(); it != files.end(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

class SignalHandlerValidator : public BaseValidator
{
protected:
    static bool isSignalHandler(const QStringRef &name)
    {
        return name.length() > 2 && name.startsWith("on") && name.at(2).isUpper();
    }

    virtual bool visit(QQmlJS::AST::UiScriptBinding *node)
    {
        QQmlJS::AST::UiQualifiedId* id = node->qualifiedId;
        if ((id && isSignalHandler(id->name)) || (id && id->next && isSignalHandler(id->next->name)))
            addError("signal handlers are not allowed", node);
        return true;
    }
};

void tst_Declarative::testSignalHandlers()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    SignalHandlerValidator validator;
    if (!validator.validate(filePath))
        QFAIL(qPrintable(validator.errors()));
}

void tst_Declarative::testSignalHandlers_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = files.begin(); it != files.end(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

QTEST_MAIN(tst_Declarative)

#include "tst_declarative.moc"
