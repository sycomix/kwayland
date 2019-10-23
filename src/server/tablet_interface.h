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

    TabletSeatInterface* seat(SeatInterface* seat);

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
    void sendProximityIn(uint32_t serial, Resource *tablet, Resource *surface);
    void sendProximityOut();
    void sendUp();
    void sendDown(uint32_t serial);
    void sendPressure(uint32_t pressure);
    void sendDistance(uint32_t distance);
    void sendTilt(uint32_t degreesX, uint32_t degreesY);
    void sendRotation(uint32_t degrees);
    void sendSlider(int32_t position);
    void sendWheel(int32_t degrees, int32_t clicks);
    void sendButton(uint32_t serial, uint32_t button, bool pressed);
    void sendFrame(uint32_t time);

    void sendMotion(const QPoint &pos);

    wl_resource* resource() const;

    quint64 hardwareSerial() const;

private:
    friend class TabletSeatInterface;
    explicit TabletToolInterface(Type type, uint32_t hsh, uint32_t hsl, uint32_t hih, uint32_t hil, const QVector<Capability> &capability, QObject *parent);
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
    explicit TabletInterface(uint32_t vendorId, uint32_t productId, const QString &name, const QStringList &paths, QObject* parent);
    class Private;
    QScopedPointer<Private> d;
};

// class TabletPadRingInterface : public QObject
// {
//     Q_OBJECT
// public:
//     virtual ~TabletPadRingInterface();
//
// private:
//     friend class Display;
//     explicit TabletPadRingInterface(Display *d, QObject *parent);
//     class Private;
//     QScopedPointer<Private> d;
// };
//
// class TabletPadStripInterface : public QObject
// {
//     Q_OBJECT
// public:
//     virtual ~TabletPadStripInterface();
//
// private:
//     friend class Display;
//     explicit TabletPadStripInterface(Display *d, QObject *parent);
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
// private:
//     friend class Display;
//     explicit TabletPadGroupInterface(Display *d, QObject *parent);
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
// private:
//     friend class Display;
//     explicit TabletPadInterface(Display *d, QObject *parent);
//     class Private;
//     QScopedPointer<Private> d;
// };

class KWAYLANDSERVER_EXPORT TabletSeatInterface : public QObject
{
    Q_OBJECT
public:
    virtual ~TabletSeatInterface();

    void addTablet(uint32_t vendorId, uint32_t productId, const QString &name, const QStringList &paths);
    void addTool(TabletToolInterface::Type type, quint64 hardwareSerial, quint64 hardwareId, const QVector<TabletToolInterface::Capability> &capabilities);

    TabletToolInterface* toolForSerialId(quint64 serialId) const;

private:
    friend class TabletManagerInterface;
    explicit TabletSeatInterface(Display* display, QObject* parent);
    class Private;
    QScopedPointer<Private> d;
};

}
}

Q_DECLARE_METATYPE(KWayland::Server::TabletSeatInterface*)

#endif
