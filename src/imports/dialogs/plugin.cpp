/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQml/qqml.h>
#include <QtQml/qqmlextensionplugin.h>
#include "qquickmessagedialog_p.h"
#include "qquickabstractmessagedialog_p.h"
#include "qquickdialogassets_p.h"
#include "qquickplatformmessagedialog_p.h"
#include "qquickfiledialog_p.h"
#include "qquickabstractfiledialog_p.h"
#include "qquickplatformfiledialog_p.h"
#include "qquickcolordialog_p.h"
#include "qquickabstractcolordialog_p.h"
#include "qquickplatformcolordialog_p.h"
#include "qquickfontdialog_p.h"
#include "qquickabstractfontdialog_p.h"
#include "qquickplatformfontdialog_p.h"
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

//#define PURE_QML_ONLY
//#define DEBUG_REGISTRATION

static void initResources()
{
    Q_INIT_RESOURCE(dialogs);
}

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule QtQuick.Dialogs 1.1
    \title Qt Quick Dialogs QML Types
    \ingroup qmlmodules
    \brief Provides QML types for standard file, color picker and message dialogs

    This QML module contains types for creating and interacting with system dialogs.

    To use the types in this module, import the module with the following line:

    \code
    import QtQuick.Dialogs 1.1
    \endcode
*/

class QtQuick2DialogsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    QtQuick2DialogsPlugin() : QQmlExtensionPlugin(), m_useResources(true) { }

    virtual void initializeEngine(QQmlEngine *engine, const char * uri) {
#ifdef DEBUG_REGISTRATION
        qDebug() << Q_FUNC_INFO << uri << m_decorationComponentUrl;
#else
        Q_UNUSED(uri)
#endif
        QQuickAbstractDialog::m_decorationComponent =
            new QQmlComponent(engine, m_decorationComponentUrl, QQmlComponent::Asynchronous);
    }

    virtual void registerTypes(const char *uri) {
        initResources();

#ifdef DEBUG_REGISTRATION
        qDebug() << Q_FUNC_INFO << uri;
#endif
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtQuick.Dialogs"));
        bool hasTopLevelWindows = QGuiApplicationPrivate::platformIntegration()->
            hasCapability(QPlatformIntegration::MultipleWindows);
        QDir qmlDir(baseUrl().toLocalFile());
        QDir widgetsDir(baseUrl().toLocalFile());
        // TODO: find the directory by searching rather than assuming a relative path
        widgetsDir.cd("../PrivateWidgets");

        // If at least one file was actually installed, then use installed qml files instead of resources.
        // This makes debugging and incremental development easier, whereas the "normal" installation
        // uses resources to save space and cut down on the number of files to deploy.
        if (qmlDir.exists(QString("DefaultFileDialog.qml")))
            m_useResources = false;
        m_decorationComponentUrl = m_useResources ?
            QUrl("qrc:/QtQuick/Dialogs/qml/DefaultWindowDecoration.qml") :
            QUrl::fromLocalFile(qmlDir.filePath(QString("qml/DefaultWindowDecoration.qml")));

        // Prefer the QPA dialog helpers if the platform supports them.
        // Else if there is a QWidget-based implementation, check whether it's
        // possible to instantiate it from Qt Quick.
        // Otherwise fall back to a pure-QML implementation.

        // MessageDialog
        qmlRegisterUncreatableType<QQuickStandardButton>(uri, 1, 1, "StandardButton",
            QLatin1String("Do not create objects of type StandardButton"));
        qmlRegisterUncreatableType<QQuickStandardIcon>(uri, 1, 1, "StandardIcon",
            QLatin1String("Do not create objects of type StandardIcon"));
#ifndef PURE_QML_ONLY
        if (QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(QPlatformTheme::MessageDialog))
            qmlRegisterType<QQuickPlatformMessageDialog>(uri, 1, 0, "MessageDialog");
        else
#endif
            registerWidgetOrQmlImplementation<QQuickMessageDialog>(widgetsDir, qmlDir, "MessageDialog", uri, hasTopLevelWindows, 1, 1);

        // FileDialog
#ifndef PURE_QML_ONLY
        if (QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(QPlatformTheme::FileDialog))
            qmlRegisterType<QQuickPlatformFileDialog>(uri, 1, 0, "FileDialog");
        else
#endif
            registerWidgetOrQmlImplementation<QQuickFileDialog>(widgetsDir, qmlDir, "FileDialog", uri, hasTopLevelWindows, 1, 0);

        // ColorDialog
#ifndef PURE_QML_ONLY
        if (QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(QPlatformTheme::ColorDialog))
            qmlRegisterType<QQuickPlatformColorDialog>(uri, 1, 0, "ColorDialog");
        else
#endif
            registerWidgetOrQmlImplementation<QQuickColorDialog>(widgetsDir, qmlDir, "ColorDialog", uri, hasTopLevelWindows, 1, 0);

        // FontDialog
#ifndef PURE_QML_ONLY
        if (QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(QPlatformTheme::FontDialog))
            qmlRegisterType<QQuickPlatformFontDialog>(uri, 1, 1, "FontDialog");
        else
#endif
            registerWidgetOrQmlImplementation<QQuickFontDialog>(widgetsDir, qmlDir, "FontDialog", uri, hasTopLevelWindows, 1, 1);
    }

protected:
    template <class WrapperType>
    void registerWidgetOrQmlImplementation(QDir widgetsDir, QDir qmlDir,
            const char *qmlName, const char *uri, bool hasTopLevelWindows, int versionMajor, int versionMinor) {
        // qDebug() << "QtQuick2DialogsPlugin::registerWidgetOrQmlImplementation" << uri << qmlName << ": QML in" << qmlDir.absolutePath()
        // << "using resources?" << m_useResources << "; widgets in" << widgetsDir.absolutePath();
        bool needQmlImplementation = true;

#ifdef PURE_QML_ONLY
        Q_UNUSED(widgetsDir)
        Q_UNUSED(hasTopLevelWindows)
#else
        // If there is a qmldir and we have a QApplication instance (as opposed to a
        // widget-free QGuiApplication), assume that the widget-based dialog will work.
        if (hasTopLevelWindows && widgetsDir.exists("qmldir") &&
                QCoreApplication::instance()->inherits("QApplication")) {
            QUrl dialogQmlPath =  m_useResources ?
                QUrl(QString("qrc:/QtQuick/Dialogs/Widget%1.qml").arg(qmlName)) :
                QUrl::fromLocalFile(qmlDir.filePath(QString("Widget%1.qml").arg(qmlName)));
            if (qmlRegisterType(dialogQmlPath, uri, versionMajor, versionMinor, qmlName) >= 0) {
                needQmlImplementation = false;
#ifdef DEBUG_REGISTRATION
                qDebug() << "    registering" << qmlName << " as " << dialogQmlPath << "success?" << !needQmlImplementation;
#endif
            }
        }
#endif
        if (needQmlImplementation) {
            QByteArray abstractTypeName = QByteArray("Abstract") + qmlName;
            qmlRegisterType<WrapperType>(uri, versionMajor, versionMinor, abstractTypeName); // implementation wrapper
            QUrl dialogQmlPath =  m_useResources ?
                QUrl(QString("qrc:/QtQuick/Dialogs/Default%1.qml").arg(qmlName)) :
                QUrl::fromLocalFile(qmlDir.filePath(QString("Default%1.qml").arg(qmlName)));
#ifdef DEBUG_REGISTRATION
            qDebug() << "    registering" << qmlName << " as " << dialogQmlPath << "success?" <<
#endif
            qmlRegisterType(dialogQmlPath, uri, versionMajor, versionMinor, qmlName);
        }
    }

    QUrl m_decorationComponentUrl;
    bool m_useResources;
};

QT_END_NAMESPACE

#include "plugin.moc"
