/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

#include "qquickstyle.h"
#include "qquickstyleattached_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qdebug.h>
#include <QtCore/qsettings.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQuickStyle
    \brief The QQQuickStyle class allows configuring the application style.
    \inmodule QtQuickControls2
    \since 5.7

    QQuickStyle provides API for querying and configuring the application
    \l {Styling Qt Quick Controls 2}{styles} of Qt Quick Controls 2.

    \code
    #include <QGuiApplication>
    #include <QQmlApplicationEngine>
    #include <QQuickStyle>

    int main(int argc, char *argv[])
    {
        QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QGuiApplication app(argc, argv);

        QQuickStyle::setStyle("Material");

        QQmlApplicationEngine engine;
        engine.load(QUrl("qrc:/main.qml"));

        return app.exec();
    }
    \endcode

    \note The style must be configured \b before loading QML that imports
    Qt Quick Controls 2. It is not possible to change the style after the QML
    types have been registered.

    \sa {Styling Qt Quick Controls 2}
*/

struct QQuickStyleSpec
{
    QQuickStyleSpec() : resolved(false) { }

    QString name()
    {
        if (!resolved)
            resolve();
        return style.mid(style.lastIndexOf(QLatin1Char('/')) + 1);
    }

    QString path()
    {
        if (!resolved)
            resolve();
        QString s = style;
        if (QQmlFile::isLocalFile(s))
            s = QQmlFile::urlToLocalFileOrQrc(s);
        return s.left(s.lastIndexOf(QLatin1Char('/')) + 1);
    }

    void setStyle(const QString &s)
    {
        style = s;
        resolved = false;
        resolve();
    }

    void resolve()
    {
        if (style.isEmpty())
            style = QGuiApplicationPrivate::styleOverride;
        if (style.isEmpty())
            style = QString::fromLatin1(qgetenv("QT_QUICK_CONTROLS_STYLE"));
        if (style.isEmpty()) {
            QSharedPointer<QSettings> settings = QQuickStyleAttached::settings(QStringLiteral("Controls"));
            if (settings)
                style = settings->value(QStringLiteral("Style")).toString();
        }

        if (QGuiApplication::instance()) {
            if (!style.contains(QLatin1Char('/'))) {
                const QString targetPath = QStringLiteral("QtQuick/Controls.2");
                const QStringList importPaths = QQmlEngine().importPathList();

                for (const QString &importPath : importPaths) {
                    QDir importDir(importPath);
                    if (importDir.cd(targetPath)) {
                        if (style.isEmpty()) {
                            style = importDir.absolutePath() + QLatin1Char('/');
                            resolved = true;
                            break;
                        }
                        const QStringList entries = importDir.entryList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);
                        for (const QString &entry : entries) {
                            if (entry.compare(style, Qt::CaseInsensitive) == 0) {
                                style = importDir.absoluteFilePath(entry);
                                resolved = true;
                                break;
                            }
                        }
                    }
                    if (resolved)
                        break;
                }
            }
            resolved = true;
        }
    }

    bool resolved;
    QString style;
};

Q_GLOBAL_STATIC(QQuickStyleSpec, styleSpec)

/*!
    Returns the name of the application style.

    \note The application style can be specified by passing a \c -style command
          line argument. Therefore \c name() may not return a fully resolved
          value if called before constructing a QGuiApplication.
*/
QString QQuickStyle::name()
{
    return styleSpec()->name();
}

/*!
    Returns the path of an overridden application style, or an empty
    string if the style is one of the built-in Qt Quick Controls 2 styles.

    \note The application style can be specified by passing a \c -style command
          line argument. Therefore \c path() may not return a fully resolved
          value if called before constructing a QGuiApplication.
*/
QString QQuickStyle::path()
{
    return styleSpec()->path();
}

/*!
    Sets the application style to \a style.

    \note The style must be configured \b before loading QML that imports Qt Quick Controls 2.
          It is not possible to change the style after the QML types have been registered.
*/
void QQuickStyle::setStyle(const QString &style)
{
    if (QQmlMetaType::isModule(QStringLiteral("QtQuick.Controls"), 2, 0)) {
        qWarning() << "ERROR: QQuickStyle::setStyle() must be called before loading QML that imports Qt Quick Controls 2.";
        return;
    }

    styleSpec()->setStyle(style);
}

QT_END_NAMESPACE
