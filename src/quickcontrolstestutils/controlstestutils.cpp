// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "controlstestutils_p.h"

#include <QtCore/qdiriterator.h>
#include <QtTest/qsignalspy.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>
#include <QtQuickTemplates2/private/qquickabstractbutton_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQuickTemplates2/private/qquickpopup_p.h>
#include <QtQuickTemplates2/private/qquickpopupitem_p_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p_p.h>

QQuickControlsTestUtils::QQuickControlsApplicationHelper::QQuickControlsApplicationHelper(QQmlDataTest *testCase,
    const QString &testFilePath, const QVariantMap &initialProperties, const QStringList &qmlImportPaths)
    : QQuickApplicationHelper(testCase, testFilePath, initialProperties, qmlImportPaths)
{
    if (ready)
        appWindow = qobject_cast<QQuickApplicationWindow*>(cleanup.data());
}

/*!
    \internal

    If \a style is different from the current style, this function will
    recreate the QML engine, clear type registrations and set the new style.

    Returns \c true if successful or if \c style is already set.
*/
bool QQuickControlsTestUtils::QQuickStyleHelper::updateStyle(const QString &style)
{
    // If it's not the first time a style has been set and the new style is not different, do nothing.
    if (!currentStyle.isEmpty() && style == currentStyle)
        return true;

    engine.reset();
    currentStyle = style;
    qmlClearTypeRegistrations();
    engine.reset(new QQmlEngine);
    QQuickStyle::setStyle(style);

    QQmlComponent component(engine.data());
    component.setData(QString::fromUtf8("import QtQuick\nimport QtQuick.Controls\n Control { }").toUtf8(), QUrl());
    if (!component.isReady())
        qWarning() << "Failed to load component:" << component.errorString();
    return component.isReady();
}

void QQuickControlsTestUtils::forEachControl(QQmlEngine *engine, const QString &qqc2ImportPath,
    const QString &styleName, const QString &targetPath, const QStringList &skipList,
    QQuickControlsTestUtils::ForEachCallback callback)
{
    // We cannot use QQmlComponent to load QML files directly from the source tree.
    // For styles that use internal QML types (eg. material/Ripple.qml), the source
    // dir would be added as an "implicit" import path overriding the actual import
    // path (qtbase/qml/QtQuick/Controls.2/Material). => The QML engine fails to load
    // the style C++ plugin from the implicit import path (the source dir).
    //
    // Therefore we only use the source tree for finding out the set of QML files that
    // a particular style implements, and then we locate the respective QML files in
    // the engine's import path. This way we can use QQmlComponent to load each QML file
    // for benchmarking.

    const QFileInfoList entries = QDir(qqc2ImportPath + QLatin1Char('/') + styleName.toLower()).entryInfoList(
        QStringList(QStringLiteral("*.qml")), QDir::Files);
    for (const QFileInfo &entry : entries) {
        QString name = entry.baseName();
        if (!skipList.contains(name)) {
            const auto importPathList = engine->importPathList();
            for (const QString &importPath : importPathList) {
                const QString relativePath = entry.dir().dirName() + QLatin1Char('/') + entry.fileName();
                QString filePath = importPath + QLatin1Char('/') + targetPath + QLatin1Char('/') + entry.fileName();
                if (filePath.startsWith(QLatin1Char(':')))
                    filePath.prepend(QStringLiteral("qrc"));
                if (QFile::exists(filePath)) {
                    callback(styleName, name, relativePath, QUrl::fromLocalFile(filePath));
                    break;
                } else {
                    QUrl url(filePath);
                    filePath = QQmlFile::urlToLocalFileOrQrc(filePath);
                    if (!filePath.isEmpty() && QFile::exists(filePath)) {
                        callback(styleName, name, relativePath, url);
                        break;
                    }
                }
            }
        }
    }
}

void QQuickControlsTestUtils::addTestRowForEachControl(QQmlEngine *engine, const QString &qqc2ImportPath,
    const QString &styleName, const QString &targetPath, const QStringList &skipList)
{
    forEachControl(engine, qqc2ImportPath, styleName, targetPath, skipList, [&](
            const QString &, const QString &, const QString &relativePath, const QUrl &absoluteUrl) {
        QTest::newRow(qPrintable(relativePath)) << absoluteUrl;
    });
}


bool QQuickControlsTestUtils::verifyButtonClickable(QQuickAbstractButton *button)
{
    if (!button->window()) {
        qWarning() << "button" << button << "doesn't have an associated window";
        return false;
    }

    if (!button->isEnabled()) {
        qWarning() << "button" << button << "is not enabled";
        return false;
    }

    if (!button->isVisible()) {
        qWarning() << "button" << button << "is not visible";
        return false;
    }

    if (button->width() <= 0.0) {
        qWarning() << "button" << button << "must have a width greater than 0";
        return false;
    }

    if (button->height() <= 0.0) {
        qWarning() << "button" << button << "must have a height greater than 0";
        return false;
    }

    return true;
}

bool QQuickControlsTestUtils::clickButton(QQuickAbstractButton *button)
{
    if (!verifyButtonClickable(button))
        return false;

    QSignalSpy spy(button, &QQuickAbstractButton::clicked);
    if (!spy.isValid()) {
        qWarning() << "Button" << button << "must have a valid clicked signal";
        return false;
    }

    const QPoint buttonCenter = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
    QTest::mouseClick(button->window(), Qt::LeftButton, Qt::NoModifier, buttonCenter);
    if (spy.size() != 1) {
        QDebug warning(QtWarningMsg);
        warning.nospace() << "The clicked signal of button " << button << " was not emitted after "
            << "clicking at " << buttonCenter << ".";
        const QQuickPopup *popup = popupParent(button);
        if (popup && !popup->isOpened()) {
            warning << " The popup it's in (" << popup << ") is no longer opened; "
                << "the click may have missed the button and gone outside of the popup, "
                << "causing it to close.";
        }
        return false;
    }

    return true;
}

/*!
    \internal

    If \a menuItem is not in a menu, use \l clickButton.
*/
bool QQuickControlsTestUtils::clickMenuItem(QQuickMenuItem *menuItem)
{
    auto *menuItemPrivate = QQuickMenuItemPrivate::get(menuItem);
    if (!menuItemPrivate->menu) {
        qWarning() << "MenuItem" << menuItem << "must be in a menu in order to be clicked";
        return false;
    }

    if (menuItemPrivate->menu->enter()) {
        /*
            FluentWinUI3 animates its height in its enter transition. This causes issues in
            context menu tests (tst_QQuickContextMenu) on Ubuntu (X11), because the native resize
            events caused by the menu's height changes arrive too late, causing clicks to miss the
            menu item and instead close the menu (which clickButton now warns about).

            There doesn't appear to be a way to reliably detect and hence wait for these events.
            We also can't disable the enter transition because the menu doesn't exist until the
            right click event, by which point the transition has also already started.

            We tried an environment variable to allow the test to disable them before they start,
            but it was still flaky. So we now simply click the menu item programmatically for menus
            with enter transitions.
        */
        menuItem->click();
        return true;
    }

    return clickButton(menuItem);
}

bool QQuickControlsTestUtils::doubleClickButton(QQuickAbstractButton *button)
{
    if (!verifyButtonClickable(button))
        return false;

    QSignalSpy spy(button, &QQuickAbstractButton::clicked);
    if (!spy.isValid()) {
        qWarning() << "button" << button << "must have a valid doubleClicked signal";
        return false;
    }

    const QPoint buttonCenter = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
    QTest::mouseDClick(button->window(), Qt::LeftButton, Qt::NoModifier, buttonCenter);
    if (spy.size() != 1) {
        qWarning() << "doubleClicked signal of button" << button << "was not emitted after double-clicking";
        return false;
    }

    return true;
}

/*!
    Allows creating QQmlComponents in C++, which is useful for tests that need
    to check if items created from the component have the correct QML context.
*/
Q_INVOKABLE QQmlComponent *QQuickControlsTestUtils::ComponentCreator::createComponent(const QByteArray &data)
{
    std::unique_ptr<QQmlComponent> component(new QQmlComponent(qmlEngine(this)));
    component->setData(data, QUrl());
    if (component->isError())
        qmlWarning(this) << "Failed to create component from the following data:\n" << data;
    return component.release();
}

QQuickControlsTestUtils::StyleInfo *QQuickControlsTestUtils::StyleInfo::create(QQmlEngine *, QJSEngine *)
{
    return StyleInfo::instance();
}

QQuickControlsTestUtils::StyleInfo *QQuickControlsTestUtils::StyleInfo::instance()
{
    static std::unique_ptr<StyleInfo> instance(new StyleInfo);
    // If this API was only used from QML, we could use the default JavaScriptOwnership
    // and let the engine take ownership of us. However, it's possible to use this API
    // only from C++, which means we need to take ownership just in case.
    QJSEngine::setObjectOwnership(instance.get(), QJSEngine::CppOwnership);
    return instance.get();
}

void QQuickControlsTestUtils::StyleInfo::initialize(const QString &controlsImportPath)
{
#if defined(Q_OS_ANDROID)
    qWarning() << "StyleInfo is not supported when cross-compiling: QTBUG-100191";
    return;
#endif

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString::fromLatin1(
        "import QtQuick.Templates; Control { }").toUtf8(), QUrl());

    const QStringList qmlTypeNames = QQmlMetaType::qmlTypeNames();

    // Collect the files from each style in the source tree.
    QDirIterator it(controlsImportPath, QStringList() << QLatin1String("*.qml") << QLatin1String("*.js"),
        QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        if (qmlTypeNames.contains(QLatin1String("QtQuick.Templates/") + info.baseName())) {
            const auto dirName = info.dir().dirName();
            const auto typeName = info.fileName();
            m_sourceQmlFiles.append({dirName, typeName, dirName + u"/" + typeName, info.filePath() });
        }
    }

    // Gather the list of styles.
    QStringList builtInStyles = QQuickStylePrivate::builtInStyles();
    // TODO: add native styles: QTBUG-87108. Originally this list was hard-coded
    // and didn't include them.
    const QStringList nativeStyles = { QLatin1String("macOS"), QLatin1String("Windows") };
    builtInStyles.removeIf([&nativeStyles](const QString &styleName){
        return nativeStyles.contains(styleName);
    });

    QList<std::pair<QString, QString>> styleRelativePaths;
    for (const auto &styleName : std::as_const(builtInStyles)) {
        // E.g. { "Basic", "QtQuick/Controls/Basic" }
        styleRelativePaths.append(std::make_pair(styleName, QLatin1String("QtQuick/Controls/") + styleName));
    }

    // Then, collect the files from each installed style directory.
    for (const auto &stylePathPair : styleRelativePaths) {
        forEachControl(&engine, controlsImportPath, stylePathPair.first, stylePathPair.second, QStringList(),
                [&](const QString &styleName, const QString &typeName, const QString &relativePath,
                    const QUrl &absoluteUrl) {
            m_installedQmlFiles.append({ styleName, typeName, relativePath, absoluteUrl.toLocalFile() });
        });
    }

    std::sort(m_sourceQmlFiles.begin(), m_sourceQmlFiles.end());
    std::sort(m_installedQmlFiles.begin(), m_installedQmlFiles.end());
}

QString QQuickControlsTestUtils::StyleInfo::styleName() const
{
    return QQuickStyle::name();
}

void QQuickControlsTestUtils::StyleInfo::warnIfNotInitialized() const
{
    if (m_sourceQmlFiles.isEmpty())
        qWarning() << "StyleInfo hasn't been initialized";
}

bool QQuickControlsTestUtils::StyleInfo::QmlFileData::operator<(
    const QQuickControlsTestUtils::StyleInfo::QmlFileData &rhs) const
{
    return relativePath < rhs.relativePath;
}

QList<QQuickControlsTestUtils::StyleInfo::QmlFileData> QQuickControlsTestUtils::StyleInfo::sourceQmlFiles() const
{
    warnIfNotInitialized();
    return m_sourceQmlFiles;
}

QList<QQuickControlsTestUtils::StyleInfo::QmlFileData> QQuickControlsTestUtils::StyleInfo::installedQmlFiles() const
{
    warnIfNotInitialized();
    return m_installedQmlFiles;
}

/*!
    It's recommended to use try-finally (see tst_monthgrid.qml for an example)
    or init/initTestCase and cleanup/cleanupTestCase if setting environment
    variables, in order to restore previous values.
*/
QString QQuickControlsTestUtils::SystemEnvironment::value(const QString &name)
{
    return QString::fromLocal8Bit(qgetenv(name.toLocal8Bit()));
}

bool QQuickControlsTestUtils::SystemEnvironment::setValue(const QString &name, const QString &value)
{
    return qputenv(name.toLocal8Bit(), value.toLocal8Bit());
}

QString QQuickControlsTestUtils::visualFocusFailureMessage(QQuickControl *control)
{
    QString message;
    QDebug debug(&message);
    const auto *controlPrivate = QQuickControlPrivate::get(control);
    const QQuickWindow *window = control->window();
    const QString activeFocusItemStr = window
        ? QDebug::toString(window->activeFocusItem()) : QStringLiteral("(unknown; control has no window)");
    debug.nospace() << "control: " << control << " activeFocus: " << control->hasActiveFocus()
        << " focusReason: " << static_cast<Qt::FocusReason>(controlPrivate->focusReason)
        << " activeFocusItem: " << activeFocusItemStr;
    return message;
}

bool QQuickControlsTestUtils::ApplicationAttributes::test(Qt::ApplicationAttribute attribute) const
{
    return QCoreApplication::testAttribute(attribute);
}

void QQuickControlsTestUtils::ApplicationAttributes::set(Qt::ApplicationAttribute attribute, bool on)
{
    QCoreApplication::setAttribute(attribute, on);
}

bool QQuickControlsTestUtils::arePopupWindowsSupported()
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_QNX) && QT_CONFIG(xcb)
    return QGuiApplication::platformName().startsWith(QLatin1String("xcb"));
#elif defined(Q_OS_WINDOWS) || defined(Q_OS_MACOS)
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::Capability::MultipleWindows);
#else
    return false;
#endif
}

/*!
    \internal

    Finds the popup that \a item is in, or returns \c nullptr.
*/
QQuickPopup *QQuickControlsTestUtils::popupParent(QQuickItem *item)
{
    QQuickItem *parentItem = item;
    while (parentItem) {
        auto *parentAsPopupItem = qobject_cast<QQuickPopupItem *>(parentItem);
        if (parentAsPopupItem)
            return QQuickPopupItemPrivate::get(parentAsPopupItem)->popup;

        parentItem = parentItem->parentItem();
    }

    return nullptr;
}

QByteArray QQuickTest::Private::qActiveFocusFailureMessage(QQuickPopup *popup)
{
    QByteArray message;
    QDebug debug(&message);
    const QQuickWindow *window = popup->window();
    const QString activeFocusItemStr = window
        ? QDebug::toString(window->activeFocusItem()) : QStringLiteral("(unknown; popup has no window)");
    debug.nospace() << "popup: " << popup;
    debug.noquote() << " window's activeFocusItem: " << activeFocusItemStr;
    return message;
}
