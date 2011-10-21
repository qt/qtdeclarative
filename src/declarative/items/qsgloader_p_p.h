// Commit: 5d2817cd668a705729df1727de49adf00713ac97
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#ifndef QSGLOADER_P_P_H
#define QSGLOADER_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsgloader_p.h"
#include "qsgimplicitsizeitem_p_p.h"
#include "qsgitemchangelistener_p.h"
#include <qdeclarativeincubator.h>

#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE


class QSGLoaderPrivate;
class QSGLoaderIncubator : public QDeclarativeIncubator
{
public:
    QSGLoaderIncubator(QSGLoaderPrivate *l, IncubationMode mode) : QDeclarativeIncubator(mode), loader(l) {}

protected:
    virtual void statusChanged(Status);
    virtual void setInitialState(QObject *);

private:
    QSGLoaderPrivate *loader;
};

class QDeclarativeContext;
class QSGLoaderPrivate : public QSGImplicitSizeItemPrivate, public QSGItemChangeListener
{
    Q_DECLARE_PUBLIC(QSGLoader)

public:
    QSGLoaderPrivate();
    ~QSGLoaderPrivate();

    void itemGeometryChanged(QSGItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
    void clear();
    void initResize();
    void load();

    void incubatorStateChanged(QDeclarativeIncubator::Status status);
    void setInitialState(QObject *o);
    void disposeInitialPropertyValues();
    QUrl resolveSourceUrl(QDeclarativeV8Function *args);
    v8::Handle<v8::Object> extractInitialPropertyValues(QDeclarativeV8Function *args, QObject *loader, bool *error);

    QUrl source;
    QSGItem *item;
    QDeclarativeComponent *component;
    QDeclarativeContext *itemContext;
    QSGLoaderIncubator *incubator;
    v8::Persistent<v8::Object> initialPropertyValues;
    v8::Persistent<v8::Object> qmlGlobalForIpv;
    bool updatingSize: 1;
    bool itemWidthValid : 1;
    bool itemHeightValid : 1;
    bool active : 1;
    bool loadingFromSource : 1;
    bool asynchronous : 1;

    void _q_sourceLoaded();
    void _q_updateSize(bool loaderGeometryChanged = true);
};

QT_END_NAMESPACE

#endif // QSGLOADER_P_P_H
