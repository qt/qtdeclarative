/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtDeclarative/QQuickView>
#include <QtGui/QGuiApplication>
#include <QtQuick1/QDeclarativeView>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    bool qtquick2 = true;
    for (int i = 1; i < app.arguments().size(); ++i) {
        const QString arg = app.arguments().at(i);
        if (arg == "-qtquick1") {
            qtquick2 = false;
        } else if (arg == "-qtquick2") {
            qtquick2 = true;
        } else {
            qWarning() << "Usage: " << app.arguments().at(0) << "[-qtquick1|-qtquick2]";
            return -1;
        }
    }

    if (qtquick2) {
        QQuickView *view = new QQuickView();
        view->setSource(QUrl::fromLocalFile("qtquick2.qml"));
    } else {
        QDeclarativeView *view = new QDeclarativeView();
        view->setSource(QUrl::fromLocalFile("qtquick1.qml"));
    }
    return app.exec();
}
