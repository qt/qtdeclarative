/****************************************************************************
**
** Copyright (C) 2016 Research In Motion.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLMODELSMODULE_H
#define QQMLMODELSMODULE_H

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

#include <QtQml/qqml.h>

#if QT_CONFIG(itemmodel)
#include <QtCore/qitemselectionmodel.h>
#endif

#include <private/qtqmlmodelsglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_QMLMODELS_PRIVATE_EXPORT QQmlModelsModule
{
public:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    static void registerQmlTypes();
    static void registerQuickTypes();
#endif
};

#if QT_CONFIG(itemmodel)
struct QItemSelectionModelForeign
{
    Q_GADGET
    QML_FOREIGN(QItemSelectionModel)
    QML_NAMED_ELEMENT(ItemSelectionModel)
    QML_ADDED_IN_MINOR_VERSION(2)
};
#endif

QT_END_NAMESPACE

#endif
