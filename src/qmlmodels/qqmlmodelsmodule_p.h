// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qitemselectionmodel.h>
#endif

#include <private/qtqmlmodelsglobal_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(itemmodel)
struct QItemSelectionModelForeign
{
    Q_GADGET
    QML_FOREIGN(QItemSelectionModel)
    QML_NAMED_ELEMENT(ItemSelectionModel)
    QML_ADDED_IN_VERSION(2, 2)
};

struct QAbstractItemModelForeign
{
    Q_GADGET
    QML_FOREIGN(QAbstractItemModel)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 15)
};
#endif

QT_END_NAMESPACE

#endif
