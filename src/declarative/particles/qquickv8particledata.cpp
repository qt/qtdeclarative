/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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

#include "qquickv8particledata_p.h"
#include "qquickparticlesystem_p.h"//for QQuickParticleData
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass Particle
    \inqmlmodule QtQuick.Particles 2
    \brief Particle elements can be manipulated in custom emitters and affectors.

    Particle elements are always managed internally by the ParticleSystem and cannot be created in QML.
    However, sometimes they are exposed via signals so as to allow arbitrary changes to the particle state
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialX
    The x coordinate of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialVX
    The x velocity of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialAX
    The x acceleration of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialY
    The y coordinate of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialVY
    The y velocity of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::initialAY
    The y acceleration of the particle at the beginning of its lifetime.

    The method of simulation prefers to have the initial values changed, rather
    than determining and changing the value at a given time. Change initial
    values in CustomEmitters instead of the current values.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::x
    The current x coordinate of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::vx
    The current x velocity of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::ax
    The current x acceleration of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::y
    The current y coordinate of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::vy
    The current y velocity of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::ay
    The current y acceleration of the particle.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::t
    The time, in seconds since the beginning of the simulation, that the particle was born.
*/


/*!
    \qmlproperty real QtQuick.Particles2::Particle::startSize
    The size in pixels that the particle image is at the start
    of its life.
*/


/*!
    \qmlproperty real QtQuick.Particles2::Particle::endSize
    The size in pixels that the particle image is at the end
    of its life. If this value is less than 0, then it is
    disregarded and the particle will have its startSize for the
    entire lifetime.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::lifeSpan
    The time in seconds that the particle will live for.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::rotation
    Degrees clockwise that the particle image is rotated at
    the beginning of its life.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::rotationSpeed
    Degrees clockwise per second that the particle image is rotated at while alive.
*/

/*!
    \qmlproperty real QtQuick.Particles2::Particle::autoRotate
    If autoRotate == 1.0, then the particle's rotation will be
    set so that it faces the direction of travel, plus any
    rotation from the rotation or rotationSpeed properties.
*/
/*!
    \qmlmethod real QtQuick.Particles2::Particle::lifeLeft
    The time in seconds that the particle has left to live at
    the current point in time.
*/
/*!
    \qmlmethod real QtQuick.Particles2::Particle::currentSize
    The currentSize of the particle, interpolating between startSize and endSize based on the currentTime.
*/



//### Particle data handles are not locked to within certain scopes like QQuickContext2D, but there's no way to reload either...
class QV8ParticleDataResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(ParticleDataType)
public:
    QV8ParticleDataResource(QV8Engine *e) : QV8ObjectResource(e) {}
    QQuickParticleData* datum;//TODO: Guard needed?
};

class QV8ParticleDataDeletable : public QV8Engine::Deletable
{
public:
    QV8ParticleDataDeletable(QV8Engine *engine);
    ~QV8ParticleDataDeletable();

    v8::Persistent<v8::Function> constructor;
};

static v8::Handle<v8::Value> particleData_discard(const v8::Arguments &args)
{
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(args.This());

    if (!r || !r->datum)
        V8THROW_ERROR("Not a valid ParticleData object");

    r->datum->lifeSpan = 0; //Don't kill(), because it could still be in the middle of being created
    return v8::Undefined();
}

static v8::Handle<v8::Value> particleData_lifeLeft(const v8::Arguments &args)
{
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(args.This());
    if (!r || !r->datum)
        V8THROW_ERROR("Not a valid ParticleData object");

    return v8::Number::New(r->datum->lifeLeft());
}

static v8::Handle<v8::Value> particleData_curSize(const v8::Arguments &args)
{
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(args.This());
    if (!r || !r->datum)
        V8THROW_ERROR("Not a valid ParticleData object");

    return v8::Number::New(r->datum->curSize());
}

#define FLOAT_GETTER_AND_SETTER(VARIABLE) static v8::Handle<v8::Value> particleData_get_ ## VARIABLE (v8::Local<v8::String>, const v8::AccessorInfo &info) \
{ \
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This()); \
    if (!r || !r->datum) \
        V8THROW_ERROR("Not a valid ParticleData object"); \
\
    return v8::Number::New(r->datum-> VARIABLE);\
}\
\
static void particleData_set_ ## VARIABLE (v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)\
{\
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This());\
    if (!r || !r->datum)\
        V8THROW_ERROR_SETTER("Not a valid ParticleData object");\
\
    r->datum-> VARIABLE = value->NumberValue();\
}

#define FAKE_FLOAT_GETTER_AND_SETTER(VARIABLE, GETTER, SETTER) static v8::Handle<v8::Value> particleData_get_ ## VARIABLE (v8::Local<v8::String>, const v8::AccessorInfo &info) \
{ \
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This()); \
    if (!r || !r->datum) \
        V8THROW_ERROR("Not a valid ParticleData object"); \
\
    return v8::Number::New(r->datum-> GETTER ());\
}\
\
static void particleData_set_ ## VARIABLE (v8::Local<v8::String>, v8::Local<v8::Value> value, const v8::AccessorInfo &info)\
{\
    QV8ParticleDataResource *r = v8_resource_cast<QV8ParticleDataResource>(info.This());\
    if (!r || !r->datum)\
        V8THROW_ERROR_SETTER("Not a valid ParticleData object");\
\
    r->datum-> SETTER ( value->NumberValue() );\
}

#define FLOAT_REGISTER_ACCESSOR(FT, ENGINE, VARIABLE, NAME) FT ->PrototypeTemplate()->SetAccessor( v8::String::New( #NAME ), particleData_get_ ## VARIABLE , particleData_set_ ## VARIABLE , v8::External::Wrap(ENGINE))

FLOAT_GETTER_AND_SETTER(x)
FLOAT_GETTER_AND_SETTER(y)
FLOAT_GETTER_AND_SETTER(t)
FLOAT_GETTER_AND_SETTER(lifeSpan)
FLOAT_GETTER_AND_SETTER(size)
FLOAT_GETTER_AND_SETTER(endSize)
FLOAT_GETTER_AND_SETTER(vx)
FLOAT_GETTER_AND_SETTER(vy)
FLOAT_GETTER_AND_SETTER(ax)
FLOAT_GETTER_AND_SETTER(ay)
FLOAT_GETTER_AND_SETTER(xx)
FLOAT_GETTER_AND_SETTER(xy)
FLOAT_GETTER_AND_SETTER(rotation)
FLOAT_GETTER_AND_SETTER(rotationSpeed)
FLOAT_GETTER_AND_SETTER(autoRotate)
FLOAT_GETTER_AND_SETTER(animIdx)
FLOAT_GETTER_AND_SETTER(frameDuration)
FLOAT_GETTER_AND_SETTER(frameCount)
FLOAT_GETTER_AND_SETTER(animT)
FLOAT_GETTER_AND_SETTER(r)
FLOAT_GETTER_AND_SETTER(update)
FAKE_FLOAT_GETTER_AND_SETTER(curX, curX, setInstantaneousX)
FAKE_FLOAT_GETTER_AND_SETTER(curVX, curVX, setInstantaneousVX)
FAKE_FLOAT_GETTER_AND_SETTER(curAX, curAX, setInstantaneousAX)
FAKE_FLOAT_GETTER_AND_SETTER(curY, curY, setInstantaneousY)
FAKE_FLOAT_GETTER_AND_SETTER(curVY, curVY, setInstantaneousVY)
FAKE_FLOAT_GETTER_AND_SETTER(curAY, curAY, setInstantaneousAY)

//TODO: Non-floats (color, update?) once floats are working well

QV8ParticleDataDeletable::QV8ParticleDataDeletable(QV8Engine *engine)
{
    v8::HandleScope handle_scope;
    v8::Context::Scope scope(engine->context());

    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->PrototypeTemplate()->Set(v8::String::New("discard"), V8FUNCTION(particleData_discard, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("lifeLeft"), V8FUNCTION(particleData_lifeLeft, engine));
    ft->PrototypeTemplate()->Set(v8::String::New("currentSize"), V8FUNCTION(particleData_curSize, engine));
    FLOAT_REGISTER_ACCESSOR(ft, engine, x, initialX);
    FLOAT_REGISTER_ACCESSOR(ft, engine, y, initialY);
    FLOAT_REGISTER_ACCESSOR(ft, engine, t, t);
    FLOAT_REGISTER_ACCESSOR(ft, engine, lifeSpan, lifeSpan);
    FLOAT_REGISTER_ACCESSOR(ft, engine, size, startSize);
    FLOAT_REGISTER_ACCESSOR(ft, engine, endSize, endSize);
    FLOAT_REGISTER_ACCESSOR(ft, engine, vx, initialVX);
    FLOAT_REGISTER_ACCESSOR(ft, engine, vy, initialVY);
    FLOAT_REGISTER_ACCESSOR(ft, engine, ax, initialAX);
    FLOAT_REGISTER_ACCESSOR(ft, engine, ay, initialAY);
    FLOAT_REGISTER_ACCESSOR(ft, engine, xx, xDeformationVector);
    FLOAT_REGISTER_ACCESSOR(ft, engine, xy, yDeformationVector);
    FLOAT_REGISTER_ACCESSOR(ft, engine, rotation, rotation);
    FLOAT_REGISTER_ACCESSOR(ft, engine, rotationSpeed, rotationSpeed);
    FLOAT_REGISTER_ACCESSOR(ft, engine, autoRotate, autoRotate);
    FLOAT_REGISTER_ACCESSOR(ft, engine, animIdx, animationIndex);
    FLOAT_REGISTER_ACCESSOR(ft, engine, frameDuration, frameDuration);
    FLOAT_REGISTER_ACCESSOR(ft, engine, frameCount, frameCount);
    FLOAT_REGISTER_ACCESSOR(ft, engine, animT, animationT);
    FLOAT_REGISTER_ACCESSOR(ft, engine, r, r);
    FLOAT_REGISTER_ACCESSOR(ft, engine, update, update);
    FLOAT_REGISTER_ACCESSOR(ft, engine, curX, x);
    FLOAT_REGISTER_ACCESSOR(ft, engine, curVX, vx);
    FLOAT_REGISTER_ACCESSOR(ft, engine, curAX, ax);
    FLOAT_REGISTER_ACCESSOR(ft, engine, curY, y);
    FLOAT_REGISTER_ACCESSOR(ft, engine, curVY, vy);
    FLOAT_REGISTER_ACCESSOR(ft, engine, curAY, ay);

    constructor = qPersistentNew(ft->GetFunction());
}

QV8ParticleDataDeletable::~QV8ParticleDataDeletable()
{
    qPersistentDispose(constructor);
}

V8_DEFINE_EXTENSION(QV8ParticleDataDeletable, particleV8Data);


QQuickV8ParticleData::QQuickV8ParticleData(QV8Engine* engine, QQuickParticleData* datum)
{
    if (!engine || !datum)
        return;
    v8::HandleScope handle_scope;
    v8::Context::Scope scope(engine->context());

    QV8ParticleDataDeletable *d = particleV8Data(engine);
    m_v8Value = qPersistentNew(d->constructor->NewInstance());
    QV8ParticleDataResource *r = new QV8ParticleDataResource(engine);
    r->datum = datum;
    m_v8Value->SetExternalResource(r);
}

QQuickV8ParticleData::~QQuickV8ParticleData()
{
    qPersistentDispose(m_v8Value);
}

QDeclarativeV8Handle QQuickV8ParticleData::v8Value()
{
    return QDeclarativeV8Handle::fromHandle(m_v8Value);
}

QT_END_NAMESPACE
