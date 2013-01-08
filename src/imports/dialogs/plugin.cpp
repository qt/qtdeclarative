/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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
#include "qquickfiledialog_p.h"
#include "qquickabstractfiledialog_p.h"
#include "qquickplatformfiledialog_p.h"
#include <private/qguiapplication_p.h>

//#define PURE_QML_ONLY

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule QtQuick.Dialogs 1
    \title Qt Quick Dialog QML Types
    \ingroup qmlmodules
    \brief Provides QML types for standard file, color picker and message dialogs

    This QML module contains types for creating and interacting with system dialogs.

    To use the types in this module, import the module with the following line:

    \code
    import QtQuick.Dialogs 1.0
    \endcode
*/

class QtQuick2DialogsPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    QtQuick2DialogsPlugin() : QQmlExtensionPlugin() { }

    virtual void initializeEngine(QQmlEngine *, const char *uri) {
        bool needQml = false;
        QDir qmlDir(baseUrl().toLocalFile());
        // If there is no support for native dialogs on the platform, we need to
        // either re-use QFileDialog, or register a QML implementation instead.
#ifdef PURE_QML_ONLY
        needQml = true;
#else
        if (!QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(QPlatformTheme::FileDialog)) {
            needQml = true;
            // If there is not a QApplication, there's no point in trying to load
            // widget-based dialogs, because a runtime error will occur.
            if (QCoreApplication::instance()->metaObject()->className() == QLatin1String("QApplication")) {
                // Test whether PrivateWidgets can load.  It's not currently possible
                // to use the given engine for that, so we need to create a temporary one.
                // That doesn't work in registerTypes either, which is why it's done here.
                QString dialogQmlPath(qmlDir.filePath("WidgetFileDialog.qml"));
                QQmlEngine tempEngine;
                QQmlComponent widgetDialogComponent(&tempEngine);
                QFile widgetDialogQmlFile(dialogQmlPath);
                widgetDialogQmlFile.open(QIODevice::ReadOnly);
                widgetDialogComponent.setData(widgetDialogQmlFile.readAll(), QUrl());

                switch (widgetDialogComponent.status()) {
                case QQmlComponent::Ready:
                    needQml = (qmlRegisterType(QUrl::fromLocalFile(dialogQmlPath), uri, 1, 0, "FileDialog") < 0);
                    // returns -1 unless we omit the module from qmldir, because otherwise
                    // QtQuick.Dialogs is already a protected namespace
                    // after the qmldir having been parsed.  (QQmlImportDatabase::importPlugin)
                    // But omitting the module from qmldir results in this warning:
                    // "Module 'QtQuick.Dialogs' does not contain a module identifier directive -
                    // it cannot be protected from external registrations."
                    // TODO register all types in registerTypes, to avoid the warning
                    // But, in that case we cannot check for existence by trying to instantiate the component.
                    // So it will have to just look for a file (qmldir?) and assume
                    // that whatever modules are installed are also in working order.
                    break;
                default:
                    break;
                }
            }
        }
#endif
        if (needQml) {
            QString dialogQmlPath = qmlDir.filePath("DefaultFileDialog.qml");
            qmlRegisterType<QQuickFileDialog>(uri, 1, 0, "AbstractFileDialog"); // implementation wrapper
            // qDebug() << "registering FileDialog as " << dialogQmlPath << "success?" <<
            qmlRegisterType(QUrl::fromLocalFile(dialogQmlPath), uri, 1, 0, "FileDialog");
        }
    }

    virtual void registerTypes(const char *uri) {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtQuick.Dialogs"));

#ifndef PURE_QML_ONLY
        // Prefer the QPA file dialog helper if the platform supports it
        if (QGuiApplicationPrivate::platformTheme()->usePlatformNativeDialog(QPlatformTheme::FileDialog))
            qmlRegisterType<QQuickPlatformFileDialog>(uri, 1, 0, "FileDialog");
#endif
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
