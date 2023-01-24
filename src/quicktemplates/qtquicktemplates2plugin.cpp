// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/private/qqmlglobal_p.h>
#include <QtQuickTemplates2/private/qtquicktemplates2global_p.h>

#if QT_CONFIG(shortcut)
#include <QtQuickTemplates2/private/qquickshortcutcontext_p_p.h>

// qtdeclarative/src/quick/util/qquickshortcut.cpp
typedef bool (*ShortcutContextMatcher)(QObject *, Qt::ShortcutContext);
extern ShortcutContextMatcher qt_quick_shortcut_context_matcher();
extern void qt_quick_set_shortcut_context_matcher(ShortcutContextMatcher matcher);
#endif

QT_BEGIN_NAMESPACE

Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Templates);
Q_GHS_KEEP_REFERENCE(QQuickTemplates_initializeModule);

class QtQuickTemplates2Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtQuickTemplates2Plugin(QObject *parent = nullptr);
    ~QtQuickTemplates2Plugin();

    void registerTypes(const char *uri) override;
    void unregisterTypes() override;

private:
#if QT_CONFIG(shortcut)
    ShortcutContextMatcher originalContextMatcher;
#endif
};

QtQuickTemplates2Plugin::QtQuickTemplates2Plugin(QObject *parent)
    : QQmlExtensionPlugin(parent)
{
    volatile auto registration = &qml_register_types_QtQuick_Templates;
    volatile auto initialization = &QQuickTemplates_initializeModule;

    Q_UNUSED(registration)
    Q_UNUSED(initialization)
}

QtQuickTemplates2Plugin::~QtQuickTemplates2Plugin()
{
    // Intentionally empty: we use register/unregisterTypes() to do
    // initialization and cleanup, as plugins are not unloaded on macOS.
}

void QtQuickTemplates2Plugin::registerTypes(const char * /*uri*/)
{
#if QT_CONFIG(shortcut)
    originalContextMatcher = qt_quick_shortcut_context_matcher();
    qt_quick_set_shortcut_context_matcher(QQuickShortcutContext::matcher);
#endif
}

void QtQuickTemplates2Plugin::unregisterTypes()
{
#if QT_CONFIG(shortcut)
    qt_quick_set_shortcut_context_matcher(originalContextMatcher);
#endif
}

QT_END_NAMESPACE

#include "qtquicktemplates2plugin.moc"
