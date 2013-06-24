/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qquickwindowmodule_p.h"
#include "qquickscreen_p.h"
#include <QtQuick/QQuickWindow>
#include <QtCore/QCoreApplication>
#include <QtQml/QQmlEngine>

QT_BEGIN_NAMESPACE

class QQuickWindowQmlImpl : public QQuickWindow, public QQmlParserStatus
{
    Q_INTERFACES(QQmlParserStatus)
    Q_OBJECT
protected:
    void classBegin() {
        //Give QQuickView behavior when created from QML with QQmlApplicationEngine
        if (QCoreApplication::instance()->property("__qml_using_qqmlapplicationengine") == QVariant(true)) {
            QQmlEngine* e = qmlEngine(this);
            if (e && !e->incubationController())
                e->setIncubationController(incubationController());
        }
    }

    void componentComplete() {}
};

void QQuickWindowModule::defineModule()
{
    const char uri[] = "QtQuick.Window";

    qmlRegisterType<QQuickWindow>(uri, 2, 0, "Window");
    qmlRegisterRevision<QWindow,1>(uri, 2, 1);
    qmlRegisterRevision<QQuickWindow,1>(uri, 2, 1);//Type moved to a subclass, but also has new members
    qmlRegisterType<QQuickWindowQmlImpl>(uri, 2, 1, "Window");
    qmlRegisterUncreatableType<QQuickScreen>(uri, 2, 0, "Screen", QStringLiteral("Screen can only be used via the attached property."));
}

#include "qquickwindowmodule.moc"

QT_END_NAMESPACE

