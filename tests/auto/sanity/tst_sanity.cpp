/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include <QtCore/private/qhooks_p.h>
#include <QtCore/qpair.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qset.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include "../../auto/shared/visualtestutil.h"

using namespace QQuickVisualTestUtil;

Q_GLOBAL_STATIC(QObjectList, qt_qobjects)

extern "C" Q_DECL_EXPORT void qt_addQObject(QObject *object)
{
    qt_qobjects->append(object);
}

extern "C" Q_DECL_EXPORT void qt_removeQObject(QObject *object)
{
    qt_qobjects->removeAll(object);
}

class tst_Sanity : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void initTestCase();

    void jsFiles();
    void functions();
    void functions_data();
    void signalHandlers();
    void signalHandlers_data();
    void anchors();
    void anchors_data();
    void attachedObjects();
    void attachedObjects_data();
    void ids();
    void ids_data();

private:
    QMap<QString, QString> sourceQmlFiles;
    QMap<QString, QString> installedQmlFiles;
    QQuickStyleHelper styleHelper;
};

void tst_Sanity::init()
{
    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&qt_addQObject);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&qt_removeQObject);
}

void tst_Sanity::cleanup()
{
    qt_qobjects->clear();
    qtHookData[QHooks::AddQObject] = 0;
    qtHookData[QHooks::RemoveQObject] = 0;
}

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
            const auto diagnosticMessages = parser.diagnosticMessages();
            for (const QQmlJS::DiagnosticMessage &msg : diagnosticMessages)
#if Q_QML_PRIVATE_API_VERSION >= 8
                m_errors += QString("%s:%d : %s").arg(m_fileName).arg(msg.loc.startLine).arg(msg.message);
#else
                m_errors += QString("%s:%d : %s").arg(m_fileName).arg(msg.line).arg(msg.message);
#endif
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

    void throwRecursionDepthError() final
    {
        m_errors += QString::fromLatin1("%1: Maximum statement or expression depth exceeded")
                            .arg(m_fileName);
    }

private:
    QString m_fileName;
    QStringList m_errors;
};

void tst_Sanity::initTestCase()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick.Templates 2.%1; Control { }").arg(15).toUtf8(), QUrl());

    const QStringList qmlTypeNames = QQmlMetaType::qmlTypeNames();

    // Collect the files from each style in the source tree.
    QDirIterator it(QQC2_IMPORT_PATH, QStringList() << "*.qml" << "*.js", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        if (qmlTypeNames.contains(QStringLiteral("QtQuick.Templates/") + info.baseName()))
            sourceQmlFiles.insert(info.dir().dirName() + "/" + info.fileName(), info.filePath());
    }

    // Then, collect the files from each installed style directory.
    const QVector<QPair<QString, QString>> styleRelativePaths = {
        { "controls/basic", "QtQuick/Controls/Basic" },
        { "controls/fusion", "QtQuick/Controls/Fusion" },
        { "controls/material", "QtQuick/Controls/Material" },
        { "controls/universal", "QtQuick/Controls/Universal" },
    };
    for (const auto &stylePathPair : styleRelativePaths) {
        forEachControl(&engine, stylePathPair.first, stylePathPair.second, QStringList(),
                [&](const QString &relativePath, const QUrl &absoluteUrl) {
             installedQmlFiles.insert(relativePath, absoluteUrl.toLocalFile());
        });
    }
}

void tst_Sanity::jsFiles()
{
    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it) {
        if (QFileInfo(it.value()).suffix() == QStringLiteral("js"))
            QFAIL(qPrintable(it.value() +  ": JS files are not allowed"));
    }
}

class FunctionValidator : public BaseValidator
{
protected:
    bool visit(QQmlJS::AST::FunctionDeclaration *node) override
    {
        addError("function declarations are not allowed", node);
        return true;
    }
};

void tst_Sanity::functions()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    FunctionValidator validator;
    if (!validator.validate(filePath))
        QFAIL(qPrintable(validator.errors()));
}

void tst_Sanity::functions_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

class SignalHandlerValidator : public BaseValidator
{
protected:
    static bool isSignalHandler(QStringView name)
    {
        return name.length() > 2 && name.startsWith(QLatin1String("on")) && name.at(2).isUpper();
    }

    bool visit(QQmlJS::AST::UiScriptBinding *node) override
    {
        QQmlJS::AST::UiQualifiedId* id = node->qualifiedId;
        if ((id && isSignalHandler(id->name)) || (id && id->next && isSignalHandler(id->next->name)))
            addError("signal handlers are not allowed", node);
        return true;
    }
};

void tst_Sanity::signalHandlers()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    SignalHandlerValidator validator;
    if (!validator.validate(filePath))
        QFAIL(qPrintable(validator.errors()));
}

void tst_Sanity::signalHandlers_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

class AnchorValidator : public BaseValidator
{
protected:
    bool visit(QQmlJS::AST::UiScriptBinding *node) override
    {
        QQmlJS::AST::UiQualifiedId* id = node->qualifiedId;
        if (id && id->name ==  QStringLiteral("anchors"))
            addError("anchors are not allowed", node);
        return true;
    }
};

void tst_Sanity::anchors()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    AnchorValidator validator;
    if (!validator.validate(filePath))
        QFAIL(qPrintable(validator.errors()));
}

void tst_Sanity::anchors_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

class IdValidator : public BaseValidator
{
public:
    IdValidator() : m_depth(0) { }

protected:
    bool visit(QQmlJS::AST::UiObjectBinding *) override
    {
        ++m_depth;
        return true;
    }

    void endVisit(QQmlJS::AST::UiObjectBinding *) override
    {
        --m_depth;
    }

    bool visit(QQmlJS::AST::UiScriptBinding *node) override
    {
        if (m_depth == 0)
            return true;

        QQmlJS::AST::UiQualifiedId *id = node->qualifiedId;
        if (id && id->name ==  QStringLiteral("id"))
            addError(QString("Internal IDs are not allowed (%1)").arg(extractName(node->statement)), node);
        return true;
    }

private:
    QString extractName(QQmlJS::AST::Statement *statement)
    {
        QQmlJS::AST::ExpressionStatement *expressionStatement = static_cast<QQmlJS::AST::ExpressionStatement *>(statement);
        if (!expressionStatement)
            return QString();

        QQmlJS::AST::IdentifierExpression *expression = static_cast<QQmlJS::AST::IdentifierExpression *>(expressionStatement->expression);
        if (!expression)
            return QString();

        return expression->name.toString();
    }

    int m_depth;
};

void tst_Sanity::ids()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    IdValidator validator;
    if (!validator.validate(filePath))
        QFAIL(qPrintable(validator.errors()));
}

void tst_Sanity::ids_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

typedef QPair<QString, QString> StringPair;
typedef QSet<StringPair> StringPairSet;

void tst_Sanity::attachedObjects()
{
    QFETCH(QStringList, ignoredAttachedClassNames);
    QFETCH(StringPairSet, expectedAttachedClassNames);

    const QString tagStr = QString::fromLatin1(QTest::currentDataTag());
    QStringList styleAndFileName = tagStr.split('/');
    QCOMPARE(styleAndFileName.size(), 2);
    QString style = styleAndFileName.first();

    if (styleHelper.updateStyle(style))
        qt_qobjects->clear();

    QString styleRelativePath = tagStr;
    styleRelativePath[0] = styleRelativePath.at(0).toLower();
    // Get the absolute path to the installed file.
    const QString controlFilePath = installedQmlFiles.value(styleRelativePath);

    QQmlComponent component(styleHelper.engine.data());
    component.loadUrl(QUrl::fromLocalFile(controlFilePath));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(object.data(), qPrintable(component.errorString()));

    // The goal of this test is to check that every unique attached type is used only once
    // within each QML file. To track this, we remove expected pairs of class names as we
    // encounter them, so that we know when something unexpected shows up.
    StringPairSet remainingAttachedClassNames = expectedAttachedClassNames;

    // Intentional copy, as QDebug creates a QObject-derived instance which would modify the list.
    const auto qobjectsCopy = *qt_qobjects;
    for (QObject *object : qobjectsCopy) {
        const QString attachedClassName = object->metaObject()->className();
        if (object->parent() == styleHelper.engine.data())
            continue; // allow "global" instances

        // objects without parents would be singletons such as QQuickFusionStyle, and we're not interested in them.
        if ((attachedClassName.endsWith("Attached") || attachedClassName.endsWith("Style")) && object->parent()) {
            QString attacheeClassName = QString::fromLatin1(object->parent()->metaObject()->className());
            const QString qmlTypeToken = QStringLiteral("QMLTYPE");
            if (attacheeClassName.contains(qmlTypeToken)) {
                // Remove the numbers from the class name, as they can change between runs; e.g.:
                // Menu_QMLTYPE_222 => Menu_QMLTYPE
                const int qmlTypeTokenIndex = attacheeClassName.indexOf(qmlTypeToken);
                QVERIFY(qmlTypeTokenIndex != -1);
                attacheeClassName = attacheeClassName.mid(0, attacheeClassName.indexOf(qmlTypeToken) + qmlTypeToken.size());
            }

            const StringPair classNamePair = { attachedClassName, attacheeClassName };
            QVERIFY2(remainingAttachedClassNames.contains(classNamePair), qPrintable(QString::fromLatin1(
                "Found an unexpected usage of an attached type: %1 is attached to %2. Either an incorrect usage was added, or the list of expected usages needs to be updated. Expected attached class names for %3 are:\n    %4")
                     .arg(attachedClassName).arg(attacheeClassName).arg(tagStr).arg(QDebug::toString(expectedAttachedClassNames))));
            remainingAttachedClassNames.remove(classNamePair);
        }
    }

    QVERIFY2(remainingAttachedClassNames.isEmpty(), qPrintable(QString::fromLatin1(
        "Not all expected attached class name usages were found; the following usages are missing:\n    %1")
            .arg(QDebug::toString(remainingAttachedClassNames))));
}

void tst_Sanity::attachedObjects_data()
{
    QTest::addColumn<QStringList>("ignoredAttachedClassNames");
    QTest::addColumn<StringPairSet>("expectedAttachedClassNames");

    QStringList ignoredNames;

    // We used to just check that there were no duplicate QMetaObject class names,
    // but that doesn't account for attached objects loaded by composite controls,
    // such as DialogButtonBox, which is loaded by Dialog.
    // So now we list all controls and the attached types we expect them to use.

    QTest::newRow("Basic/AbstractButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Action.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ActionGroup.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ApplicationWindow.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/BusyIndicator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Button.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ButtonGroup.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/CheckBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/CheckDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ComboBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Container.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Control.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/DelayButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Dial.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Dialog.qml") << ignoredNames << StringPairSet {{ "QQuickOverlayAttached", "Dialog_QMLTYPE" }};
    QTest::newRow("Basic/DialogButtonBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Drawer.qml") << ignoredNames << StringPairSet {{ "QQuickOverlayAttached", "Drawer_QMLTYPE" }};
    QTest::newRow("Basic/Frame.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/GroupBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/HorizontalHeaderView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ItemDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Label.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Menu.qml") << ignoredNames << StringPairSet {
        { "QQuickOverlayAttached", "Menu_QMLTYPE" },
        { "QQuickScrollIndicatorAttached", "QQuickListView" },
        { "QQuickWindowAttached", "QQuickListView" }
    };
    QTest::newRow("Basic/MenuBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/MenuBarItem.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/MenuItem.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/MenuSeparator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Page.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/PageIndicator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Pane.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Popup.qml") << ignoredNames << StringPairSet {{ "QQuickOverlayAttached", "Popup_QMLTYPE" }};
    QTest::newRow("Basic/ProgressBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/RadioButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/RadioDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/RangeSlider.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/RoundButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ScrollBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ScrollIndicator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ScrollView.qml") << ignoredNames << StringPairSet {{ "QQuickScrollBarAttached", "ScrollView_QMLTYPE" }};
    QTest::newRow("Basic/Slider.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/SpinBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/SplitView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/StackView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/SwipeDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/SwipeView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Switch.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/SwitchDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/TabBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/TabButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/TextArea.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/TextField.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ToolBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ToolButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ToolSeparator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/ToolTip.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/Tumbler.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Basic/VerticalHeaderView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/ApplicationWindow.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/BusyIndicator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Button.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/CheckBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/CheckDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/ComboBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/DelayButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Dial.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Dialog.qml") << ignoredNames << StringPairSet {{ "QQuickOverlayAttached", "Dialog_QMLTYPE" }};
    QTest::newRow("Fusion/DialogButtonBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Drawer.qml") << ignoredNames << StringPairSet {{ "QQuickOverlayAttached", "Drawer_QMLTYPE" }};
    QTest::newRow("Fusion/Frame.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/GroupBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/HorizontalHeaderView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/ItemDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Label.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Menu.qml") << ignoredNames << StringPairSet {
        { "QQuickOverlayAttached", "Menu_QMLTYPE" },
        { "QQuickScrollIndicatorAttached", "QQuickListView" },
        { "QQuickWindowAttached", "QQuickListView" }
    };
    QTest::newRow("Fusion/MenuBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/MenuBarItem.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/MenuItem.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/MenuSeparator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Page.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/PageIndicator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Pane.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Popup.qml") << ignoredNames << StringPairSet {{ "QQuickOverlayAttached", "Popup_QMLTYPE" }};
    QTest::newRow("Fusion/ProgressBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/RadioButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/RadioDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/RangeSlider.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/RoundButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/ScrollBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/ScrollIndicator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Slider.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/SpinBox.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/SplitView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/SwipeDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Switch.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/SwitchDelegate.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/TabBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/TabButton.qml") << ignoredNames << StringPairSet {{ "QQuickTabBarAttached", "TabButton_QMLTYPE" }};
    QTest::newRow("Fusion/TextArea.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/TextField.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/ToolBar.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/ToolButton.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/ToolSeparator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/ToolTip.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/Tumbler.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Fusion/VerticalHeaderView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Material/ApplicationWindow.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ApplicationWindow_QMLTYPE" }};
    QTest::newRow("Material/BusyIndicator.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "BusyIndicator_QMLTYPE" }};
    QTest::newRow("Material/Button.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "Button_QMLTYPE" }};
    QTest::newRow("Material/CheckBox.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "CheckBox_QMLTYPE" }};
    QTest::newRow("Material/CheckDelegate.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "CheckDelegate_QMLTYPE" }};
    QTest::newRow("Material/ComboBox.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ComboBox_QMLTYPE" }};
    QTest::newRow("Material/DelayButton.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "DelayButton_QMLTYPE" }};
    QTest::newRow("Material/Dial.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "Dial_QMLTYPE" }};
    QTest::newRow("Material/Dialog.qml") << ignoredNames << StringPairSet {
        { "QQuickMaterialStyle", "DialogButtonBox_QMLTYPE" },
        { "QQuickOverlayAttached", "Dialog_QMLTYPE" },
        { "QQuickMaterialStyle", "Dialog_QMLTYPE" },
        { "QQuickMaterialStyle", "Label_QMLTYPE" }
    };
    QTest::newRow("Material/DialogButtonBox.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "DialogButtonBox_QMLTYPE" }};
    QTest::newRow("Material/Drawer.qml") << ignoredNames << StringPairSet {
        { "QQuickOverlayAttached", "Drawer_QMLTYPE" },
        { "QQuickMaterialStyle", "Drawer_QMLTYPE" }
    };
    QTest::newRow("Material/Frame.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "Frame_QMLTYPE" }};
    QTest::newRow("Material/GroupBox.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "GroupBox_QMLTYPE" }};
    QTest::newRow("Material/HorizontalHeaderView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Material/ItemDelegate.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ItemDelegate_QMLTYPE" }};
    QTest::newRow("Material/Label.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "Label_QMLTYPE" }};
    QTest::newRow("Material/Menu.qml") << ignoredNames << StringPairSet {
        { "QQuickOverlayAttached", "Menu_QMLTYPE" },
        { "QQuickMaterialStyle", "Menu_QMLTYPE" },
        { "QQuickScrollIndicatorAttached", "QQuickListView" },
        { "QQuickWindowAttached", "QQuickListView" },
        { "QQuickMaterialStyle", "ScrollIndicator_QMLTYPE" }
    };
    QTest::newRow("Material/MenuBar.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "MenuBar_QMLTYPE" }};
    QTest::newRow("Material/MenuBarItem.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "MenuBarItem_QMLTYPE" }};
    QTest::newRow("Material/MenuItem.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "MenuItem_QMLTYPE" }};
    QTest::newRow("Material/MenuSeparator.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "MenuSeparator_QMLTYPE" }};
    QTest::newRow("Material/Page.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "Page_QMLTYPE" }};
    QTest::newRow("Material/PageIndicator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Material/Pane.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "Pane_QMLTYPE" }};
    QTest::newRow("Material/Popup.qml") << ignoredNames << StringPairSet {
        { "QQuickOverlayAttached", "Popup_QMLTYPE" },
        { "QQuickMaterialStyle", "Popup_QMLTYPE" }
    };
    QTest::newRow("Material/ProgressBar.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ProgressBar_QMLTYPE" }};
    QTest::newRow("Material/RadioButton.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "RadioButton_QMLTYPE" }};
    QTest::newRow("Material/RadioDelegate.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "RadioDelegate_QMLTYPE" }};
    QTest::newRow("Material/RangeSlider.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "RangeSlider_QMLTYPE" }};
    QTest::newRow("Material/RoundButton.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "RoundButton_QMLTYPE" }};
    QTest::newRow("Material/ScrollBar.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ScrollBar_QMLTYPE" }};
    QTest::newRow("Material/ScrollIndicator.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ScrollIndicator_QMLTYPE" }};
    QTest::newRow("Material/Slider.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "Slider_QMLTYPE" }};
    QTest::newRow("Material/SpinBox.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "SpinBox_QMLTYPE" }};
    QTest::newRow("Material/SplitView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Material/StackView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Material/SwipeDelegate.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "SwipeDelegate_QMLTYPE" }};
    QTest::newRow("Material/SwipeView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Material/Switch.qml") << ignoredNames << StringPairSet {
        { "QQuickMaterialStyle", "SwitchIndicator_QMLTYPE" },
        { "QQuickMaterialStyle", "Switch_QMLTYPE" }
    };
    QTest::newRow("Material/SwitchDelegate.qml") << ignoredNames << StringPairSet {
        { "QQuickMaterialStyle", "SwitchDelegate_QMLTYPE" },
        { "QQuickMaterialStyle", "SwitchIndicator_QMLTYPE" }
    };
    QTest::newRow("Material/TabBar.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "TabBar_QMLTYPE" }};
    QTest::newRow("Material/TabButton.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "TabButton_QMLTYPE" }};
    QTest::newRow("Material/TextArea.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "TextArea_QMLTYPE" }};
    QTest::newRow("Material/TextField.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "TextField_QMLTYPE" }};
    QTest::newRow("Material/ToolBar.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ToolBar_QMLTYPE" }};
    QTest::newRow("Material/ToolButton.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ToolButton_QMLTYPE" }};
    QTest::newRow("Material/ToolSeparator.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ToolSeparator_QMLTYPE" }};
    QTest::newRow("Material/ToolTip.qml") << ignoredNames << StringPairSet {{ "QQuickMaterialStyle", "ToolTip_QMLTYPE" }};
    QTest::newRow("Material/Tumbler.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Material/VerticalHeaderView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Universal/ApplicationWindow.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ApplicationWindow_QMLTYPE" }};
    QTest::newRow("Universal/BusyIndicator.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "BusyIndicator_QMLTYPE" }};
    QTest::newRow("Universal/Button.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "Button_QMLTYPE" }};
    QTest::newRow("Universal/CheckBox.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "CheckBox_QMLTYPE" }};
    QTest::newRow("Universal/CheckDelegate.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "CheckDelegate_QMLTYPE" }};
    QTest::newRow("Universal/ComboBox.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ComboBox_QMLTYPE" }};
    QTest::newRow("Universal/DelayButton.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "DelayButton_QMLTYPE" }};
    QTest::newRow("Universal/Dial.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "Dial_QMLTYPE" }};
    QTest::newRow("Universal/Dialog.qml") << ignoredNames << StringPairSet {
        { "QQuickOverlayAttached", "Dialog_QMLTYPE" },
        { "QQuickUniversalStyle", "Label_QMLTYPE" },
        { "QQuickUniversalStyle", "Dialog_QMLTYPE" },
        { "QQuickUniversalStyle", "DialogButtonBox_QMLTYPE" }
    };
    QTest::newRow("Universal/DialogButtonBox.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "DialogButtonBox_QMLTYPE" }};
    QTest::newRow("Universal/Drawer.qml") << ignoredNames << StringPairSet {
        { "QQuickOverlayAttached", "Drawer_QMLTYPE" },
        { "QQuickUniversalStyle", "Drawer_QMLTYPE" }
    };
    QTest::newRow("Universal/Frame.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "Frame_QMLTYPE" }};
    QTest::newRow("Universal/GroupBox.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "GroupBox_QMLTYPE" }};
    QTest::newRow("Universal/HorizontalHeaderView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Universal/ItemDelegate.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ItemDelegate_QMLTYPE" }};
    QTest::newRow("Universal/Label.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "Label_QMLTYPE" }};
    QTest::newRow("Universal/Menu.qml") << ignoredNames << StringPairSet {
        { "QQuickOverlayAttached", "Menu_QMLTYPE" },
        { "QQuickUniversalStyle", "Menu_QMLTYPE" },
        { "QQuickScrollIndicatorAttached", "QQuickListView" },
        { "QQuickWindowAttached", "QQuickListView" },
        { "QQuickUniversalStyle", "ScrollIndicator_QMLTYPE" }
    };
    QTest::newRow("Universal/MenuBar.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "MenuBar_QMLTYPE" }};
    QTest::newRow("Universal/MenuBarItem.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "MenuBarItem_QMLTYPE" }};
    QTest::newRow("Universal/MenuItem.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "MenuItem_QMLTYPE" }};
    QTest::newRow("Universal/MenuSeparator.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "MenuSeparator_QMLTYPE" }};
    QTest::newRow("Universal/Page.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "Page_QMLTYPE" }};
    QTest::newRow("Universal/PageIndicator.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Universal/Pane.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "Pane_QMLTYPE" }};
    QTest::newRow("Universal/Popup.qml") << ignoredNames << StringPairSet {
        { "QQuickOverlayAttached", "Popup_QMLTYPE" },
        { "QQuickUniversalStyle", "Popup_QMLTYPE" }
    };
    QTest::newRow("Universal/ProgressBar.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ProgressBar_QMLTYPE" }};
    QTest::newRow("Universal/RadioButton.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "RadioButton_QMLTYPE" }};
    QTest::newRow("Universal/RadioDelegate.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "RadioDelegate_QMLTYPE" }};
    QTest::newRow("Universal/RangeSlider.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "RangeSlider_QMLTYPE" }};
    QTest::newRow("Universal/RoundButton.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "RoundButton_QMLTYPE" }};
    QTest::newRow("Universal/ScrollBar.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ScrollBar_QMLTYPE" }};
    QTest::newRow("Universal/ScrollIndicator.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ScrollIndicator_QMLTYPE" }};
    QTest::newRow("Universal/Slider.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "Slider_QMLTYPE" }};
    QTest::newRow("Universal/SpinBox.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "SpinBox_QMLTYPE" }};
    QTest::newRow("Universal/SplitView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Universal/StackView.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Universal/SwipeDelegate.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "SwipeDelegate_QMLTYPE" }};
    QTest::newRow("Universal/Switch.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "Switch_QMLTYPE" }};
    QTest::newRow("Universal/SwitchDelegate.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "SwitchDelegate_QMLTYPE" }};
    QTest::newRow("Universal/TabBar.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "TabBar_QMLTYPE" }};
    QTest::newRow("Universal/TabButton.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "TabButton_QMLTYPE" }};
    QTest::newRow("Universal/TextArea.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "TextArea_QMLTYPE" }};
    QTest::newRow("Universal/TextField.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "TextField_QMLTYPE" }};
    QTest::newRow("Universal/ToolBar.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ToolBar_QMLTYPE" }};
    QTest::newRow("Universal/ToolButton.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ToolButton_QMLTYPE" }};
    QTest::newRow("Universal/ToolSeparator.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ToolSeparator_QMLTYPE" }};
    QTest::newRow("Universal/ToolTip.qml") << ignoredNames << StringPairSet {{ "QQuickUniversalStyle", "ToolTip_QMLTYPE" }};
    QTest::newRow("Universal/Tumbler.qml") << ignoredNames << StringPairSet {};
    QTest::newRow("Universal/VerticalHeaderView.qml") << ignoredNames << StringPairSet {};
}

QTEST_MAIN(tst_Sanity)

#include "tst_sanity.moc"
