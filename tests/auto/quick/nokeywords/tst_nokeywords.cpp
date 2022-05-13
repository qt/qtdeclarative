// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#define QT_NO_KEYWORDS
#undef signals
#undef slots
#undef emit
#define signals FooBar
#define slots Baz
#define emit Yoyodyne

#include <QtQuick/QtQuick>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemview_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquickpainteditem_p.h>
#include <QtQuick/private/qquickshadereffect_p.h>
#include <QtQuick/private/qquickshadereffectsource_p.h>
#include <QtQuick/private/qquickview_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qsgadaptationlayer_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qsgcontextplugin_p.h>
#if QT_CONFIG(opengl)
#include <QtQuick/private/qsgdefaultglyphnode_p.h>
#include <QtQuick/private/qsgdefaultinternalimagenode_p.h>
#include <QtQuick/private/qsgdefaultinternalrectanglenode_p.h>
#include <QtQuick/private/qsgdistancefieldglyphnode_p.h>
#endif
#include <QtQuick/private/qsggeometry_p.h>
#include <QtQuick/private/qsgnode_p.h>
#include <QtQuick/private/qsgnodeupdater_p.h>
#include <QtQuick/private/qsgdefaultpainternode_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qsgrenderloop_p.h>
#include <QtQuick/private/qsgrendernode_p.h>
#include <QtQuick/private/qsgtexturematerial_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgplaintexture_p.h>
#include <QtQuick/private/qsgthreadedrenderloop_p.h>

#include <QtQuick/private/qsgrhiatlastexture_p.h>
#include <QtQuick/private/qsgrhidistancefieldglyphcache_p.h>
#include <QtQuick/private/qsgrhilayer_p.h>
#include <QtQuick/private/qsgrhishadereffectnode_p.h>
#include <QtQuick/private/qsgrhitextureglyphcache_p.h>

#undef signals
#undef slots
#undef emit

class MyBooooooostishClass : public QObject
{
    Q_OBJECT
public:
    inline MyBooooooostishClass() {}

Q_SIGNALS:
    void mySignal();

public Q_SLOTS:
    inline void mySlot()
    {
        Q_UNUSED(signals);
        Q_UNUSED(slots);

        mySignal();
    }

private:
    int signals;
    double slots;
};

#define signals public
#define slots
#define emit
#undef QT_NO_KEYWORDS

#include <QtTest/QtTest>

class tst_NoKeywords : public QObject
{
    Q_OBJECT
};

QTEST_MAIN(tst_NoKeywords)

#include "tst_nokeywords.moc"
