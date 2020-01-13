/********************************************************************
Copyright 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#ifndef WAYLAND_SERVER_TABLET_INTERFACE_H
#define WAYLAND_SERVER_TABLET_INTERFACE_H

#include <KWayland/Server/kwaylandserver_export.h>
#include <QVector>

#include "resource.h"

namespace KWayland
{
namespace Server
{

class TabletSeatInterface;
class Display;
class SeatInterface;

class KWAYLANDSERVER_EXPORT TabletManagerInterface : public QObject
{
    Q_OBJECT
public:
    virtual ~TabletManagerInterface();

    TabletSeatInterface* seat(SeatInterface* seat) const;

private:
    friend class Display;
    explicit TabletManagerInterface(Display *d, QObject *parent);
    class Private;
    QScopedPointer<Private> d;
};

class KWAYLANDSERVER_EXPORT TabletToolInterface : public QObject
{
    Q_OBJECT
public:
    virtual ~TabletToolInterface();

    enum Type {
        Pen = 0x140, // Pen
        Eraser = 0x141, // Eraser
        Brush = 0x142, // Brush
        Pencil = 0x143, // Pencil
        Airbrush = 0x144, // Airbrush
        Finger = 0x145, // Finger
        Mouse = 0x146, // Mouse
        Lens = 0x147, // Lens
        Totem
    };

    enum Capability {
        Tilt = 1, // Tilt axes
        Pressure = 2, // Pressure axis
        Distance = 3, // Distance axis
        Rotation = 4, // Z-rotation axis
        Slider = 5, // Slider axis
        Wheel = 6 // Wheel axis
    };

    void sendRemoved();
    void sendProximityIn(quint32 serial, Resource *tablet, Resource *surface);
    void sendProximityOut();
    void sendUp();
    void sendDown(quint32 serial);
    void sendPressure(quint32 pressure);
    void sendDistance(quint32 distance);
    void sendTilt(quint32 degreesX, quint32 degreesY);
    void sendRotation(quint32 degrees);
    void sendSlider(qint32 position);
    void sendWheel(qint32 degrees, qint32 clicks);
    void sendButton(quint32 serial, quint32 button, bool pressed);
    void sendFrame(quint32 time);

    void sendMotion(const QPoint &pos);

    wl_resource* resource() const;

    quint64 hardwareSerial() const;

private:
    friend class TabletSeatInterface;
    explicit TabletToolInterface(Type type, quint32 hsh, quint32 hsl, quint32 hih, quint32 hil, const QVector<Capability> &capability, QObject *parent);
    class Private;
    QScopedPointer<Private> d;
};

class KWAYLANDSERVER_EXPORT TabletInterface : public QObject
{
    Q_OBJECT
public:
    virtual ~TabletInterface();

    wl_resource* resource() const;

private:
    friend class TabletSeatInterface;
    explicit TabletInterface(quint32 vendorId, quint32 productId, const QString &name, const QStringList &paths, QObject* parent);
    class Private;
    QScopedPointer<Private> d;
};

// class TabletPadRingInterface : public QObject
// {
//     Q_OBJECT
// public:
//     virtual ~TabletPadRingInterface();
//
//     enum Source { Finger = 1 };
//     void sendSource(Source source);
//     void sendAngle(quint32 degrees);
//     void sendFrame(quint32 time);
//     void sendStop();
//
// Q_SIGNALS:
//     void descriptionChanged(const QString &description);
//
// private:
//     explicit TabletPadRingInterface(QObject *parent);
//     class Private;
//     QScopedPointer<Private> d;
// };
//
// class TabletPadStripInterface : public QObject
// {
//     Q_OBJECT
//     Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)
// public:
//     virtual ~TabletPadStripInterface();
//     QString description() const;
//
//     enum Source { Finger = 1 };
//     void sendSource(Source source);
//     void sendPosition(quint32 position);
//     void sendFrame(quint32 timestamp);
//     void sendStop();
//
// Q_SIGNALS:
//     void descriptionChanged(const QString &description);
//
// private:
//     explicit TabletPadStripInterface(QObject *parent);
//     class Private;
//     QScopedPointer<Private> d;
// };
//
// class TabletPadInterface : public QObject
// {
//     Q_OBJECT
// public:
//     virtual ~TabletPadInterface();
//
//     void sendEnter(quint32 serial, struct ::wl_resource *tablet, struct ::wl_resource *surface);
//     void sendLeave(quint32 serial, struct ::wl_resource *surface);
//
// private:
//     explicit TabletPadInterface(const QStringList & paths, QObject *parent);
//     class Private;
//     QScopedPointer<Private> d;
// };
//
// class TabletPadGroupInterface : public QObject
// {
//     Q_OBJECT
// public:
//     virtual ~TabletPadGroupInterface();
//
//     void modeSwitch(uint32_t time, uint32_t serial, uint32_t mode);
//
// private:
//     explicit TabletPadGroupInterface(TabletPadInterface* pad, QObject *parent);
//     class Private;
//     QScopedPointer<Private> d;
// };

class KWAYLANDSERVER_EXPORT TabletSeatInterface : public QObject
{
    Q_OBJECT
public:
    virtual ~TabletSeatInterface();

    void addTablet(quint32 vendorId, quint32 productId, const QString &name, const QStringList &paths);
    void addTool(TabletToolInterface::Type type, quint64 hardwareSerial, quint64 hardwareId, const QVector<TabletToolInterface::Capability> &capabilities);

    TabletToolInterface* toolForHardwareId(quint64 hardwareId) const;

private:
    friend class TabletManagerInterface;
    explicit TabletSeatInterface(QObject* parent);
    class Private;
    QScopedPointer<Private> d;
};

}
}

Q_DECLARE_METATYPE(KWayland::Server::TabletSeatInterface*)

#endif
