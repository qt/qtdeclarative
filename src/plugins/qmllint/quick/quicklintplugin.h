// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QUICKLINTPLUGIN_H
#define QUICKLINTPLUGIN_H

#include <QtCore/qplugin.h>
#include <QtCore/qlist.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qhash.h>

#include <QtQmlCompiler/private/qqmlsa_p.h>

QT_BEGIN_NAMESPACE

class QmlLintQuickPlugin : public QObject, public QQmlSA::LintPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QmlLintPluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(QQmlSA::LintPlugin)

public:
    void registerPasses(QQmlSA::PassManager *manager, const QQmlSA::Element &rootElement) override;
};

class ForbiddenChildrenPropertyValidatorPass : public QQmlSA::ElementPass
{
public:
    ForbiddenChildrenPropertyValidatorPass(QQmlSA::PassManager *manager);

    void addWarning(QAnyStringView moduleName, QAnyStringView typeName, QAnyStringView propertyName,
                    QAnyStringView warning);
    bool shouldRun(const QQmlSA::Element &element) override;
    void run(const QQmlSA::Element &element) override;

private:
    struct Warning
    {
        QString propertyName;
        QString message;
    };

    QHash<QQmlSA::Element, QVarLengthArray<Warning, 8>> m_types;
};

class AttachedPropertyTypeValidatorPass : public QQmlSA::PropertyPass
{
public:
    struct TypeDescription
    {
        QString module;
        QString name;
    };

    AttachedPropertyTypeValidatorPass(QQmlSA::PassManager *manager);

    QString addWarning(TypeDescription attachType, QList<TypeDescription> allowedTypes,
                       bool allowInDelegate, QAnyStringView warning);

    void onBinding(const QQmlSA::Element &element, const QString &propertyName,
                   const QQmlJSMetaPropertyBinding &binding, const QQmlSA::Element &bindingScope,
                   const QQmlSA::Element &value) override;
    void onRead(const QQmlSA::Element &element, const QString &propertyName,
                const QQmlSA::Element &readScope, QQmlJS::SourceLocation location) override;
    void onWrite(const QQmlSA::Element &element, const QString &propertyName,
                 const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                 QQmlJS::SourceLocation location) override;

private:
    void checkWarnings(const QQmlSA::Element &element, const QQmlSA::Element &scopeUsedIn,
                       const QQmlJS::SourceLocation &location);

    struct Warning
    {
        QVarLengthArray<QQmlSA::Element, 4> allowedTypes;
        bool allowInDelegate = false;
        QString message;
    };
    QHash<QString, Warning> m_attachedTypes;
};

class ControlsNativeValidatorPass : public QQmlSA::ElementPass
{
public:
    ControlsNativeValidatorPass(QQmlSA::PassManager *manager);

    bool shouldRun(const QQmlSA::Element &element) override;
    void run(const QQmlSA::Element &element) override;

private:
    struct ControlElement
    {
        QString name;
        QStringList restrictedProperties;
        bool isInModuleControls = true;
        bool isControl = false;
        bool inheritsControl = false;
        QQmlSA::Element element = {};
    };

    QList<ControlElement> m_elements;
};

class AnchorsValidatorPass : public QQmlSA::ElementPass
{
public:
    AnchorsValidatorPass(QQmlSA::PassManager *manager);

    bool shouldRun(const QQmlSA::Element &element) override;
    void run(const QQmlSA::Element &element) override;

private:
    QQmlSA::Element m_item;
};

class ControlsSwipeDelegateValidatorPass : public QQmlSA::ElementPass
{
public:
    ControlsSwipeDelegateValidatorPass(QQmlSA::PassManager *manager);

    bool shouldRun(const QQmlSA::Element &element) override;
    void run(const QQmlSA::Element &element) override;

private:
    QQmlSA::Element m_swipeDelegate;
};

QT_END_NAMESPACE

#endif // QUICKLINTPLUGIN_H
