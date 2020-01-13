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
#include "tablet_interface.h"
#include "resource_p.h"
#include "seat_interface.h"
#include "display.h"
#include "surface_interface.h"

#include <QHash>
#include "qwayland-server-tablet-unstable-v2.h"

using namespace KWayland;
using namespace Server;

static int s_version = 1;

class TabletSeatInterface::Private : public QtWaylandServer::zwp_tablet_seat_v2
{
public:
    Private()
        : zwp_tablet_seat_v2()
    {}

    void zwp_tablet_seat_v2_bind_resource(Resource *resource) override {
        for (auto iface : qAsConst(m_tools))
            send_tool_added(resource->handle, iface->resource());
        for (auto iface : qAsConst(m_tablets))
            send_tablet_added(resource->handle, iface->resource());
            ;
//         for (auto r : qAsConst(m_pads))
//             send_pad_added(resource->handle, r->resource());
    }

    QVector<TabletToolInterface*> m_tools;
    QVector<TabletInterface*> m_tablets;
};

TabletSeatInterface::TabletSeatInterface(QObject* parent)
    : QObject(parent)
    , d(new Private)
{}

TabletSeatInterface::~TabletSeatInterface() = default;

void TabletSeatInterface::addTool(TabletToolInterface::Type type, quint64 hardwareSerial, quint64 hardwareId, const QVector<TabletToolInterface::Capability> &capabilities)
{
    const auto MAX_UINT_32 = std::numeric_limits<quint32>::max();
    auto iface = new TabletToolInterface(type, hardwareSerial >> 32, hardwareSerial & MAX_UINT_32,
                                               hardwareId >> 32, hardwareId & MAX_UINT_32, capabilities, this);
    for (auto r : d->resourceMap())
        d->send_tool_added(r->handle);

    QObject::connect(iface, &QObject::destroyed, this,
        [this] (QObject* object) { d->m_tools.removeAll(static_cast<TabletToolInterface*>(object)); }
    );
}

void TabletSeatInterface::addTablet(uint32_t vendorId, uint32_t productId, const QString &name, const QStringList &paths)
{
    auto iface = new TabletInterface(vendorId, productId, name, paths, this);
    for (auto r : d->resourceMap())
        d->send_tablet_added(r->handle);

    QObject::connect(iface, &QObject::destroyed, this,
        [this] (QObject* object) { d->m_tablets.removeAll(static_cast<TabletInterface*>(object)); }
    );
}

class TabletManagerInterface::Private : public QtWaylandServer::zwp_tablet_manager_v2
{
public:
    Private(Display *display, TabletManagerInterface* q)
        : zwp_tablet_manager_v2(*display, s_version)
        , q(q)
    {}

    void zwp_tablet_manager_v2_get_tablet_seat(Resource * resource, uint32_t tablet_seat, struct ::wl_resource * seat_resource) override {
        auto seat = SeatInterface::get(seat_resource);
        qCritical() << "get tablet seat" << resource << tablet_seat << seat;
        TabletSeatInterface* tsi = get(seat);
        tsi->d->add(resource->client(), tablet_seat, s_version);
    }

    TabletSeatInterface* get(SeatInterface* seat)
    {
        auto& tabletSeat = m_seats[seat];
        if (!tabletSeat) {
            tabletSeat = new TabletSeatInterface(q);
        }
        return tabletSeat;
    }

    TabletManagerInterface* const q;
    QHash<SeatInterface*, TabletSeatInterface*> m_seats;
};

TabletManagerInterface::TabletManagerInterface(Display *display, QObject *parent)
    : QObject(parent)
    , d(new Private(display, this))
{
}

TabletSeatInterface* TabletManagerInterface::seat(SeatInterface* seat) const
{
    return d->m_seats.value(seat);
}

TabletManagerInterface::~TabletManagerInterface() = default;

// TabletSeatInterface* TabletManagerInterface::seat(SeatInterface* seat)
// {
//     return d->get(seat);
// }

//
// void TabletManagerInterface::cancel()
// {
//     Q_D();
//     if (!d->resource) {
//         return;
//     }
//     wl_tablet_send_cancel(d->resource);
//     d->client->flush();
// }
//
// TabletManagerInterface::Private *TabletManagerInterface::d_func() const
// {
//     return reinterpret_cast<Private*>(d.data());
// }

class TabletToolInterface::Private : public QtWaylandServer::zwp_tablet_tool_v2
{
public:
    Private(Type type, uint32_t hsh, uint32_t hsl, uint32_t hih, uint32_t hil, const QVector<Capability>& capabilities)
        : zwp_tablet_tool_v2()
        , m_type(type)
        , m_hardwareSerialHigh(hsh)
        , m_hardwareSerialLow(hsl)
        , m_hardwareIdHigh(hih)
        , m_hardwareIdLow(hil)
        , m_capabilities(capabilities)
    {}

    void zwp_tablet_tool_v2_bind_resource(Resource * /*resource*/) override {
        send_type(m_type);
        send_hardware_serial(m_hardwareSerialHigh, m_hardwareSerialLow);
        send_hardware_id_wacom(m_hardwareIdHigh, m_hardwareIdLow);
        for (uint32_t cap : qAsConst(m_capabilities))
            send_capability(cap);
        send_done();
    }

    const uint32_t m_type;
    const uint32_t m_hardwareSerialHigh, m_hardwareSerialLow;
    const uint32_t m_hardwareIdHigh, m_hardwareIdLow;
    const QVector<Capability> m_capabilities;
};

TabletToolInterface::TabletToolInterface(Type type, uint32_t hsh, uint32_t hsl, uint32_t hih, uint32_t hil, const QVector<Capability>& capabilities, QObject *parent)
    : QObject(parent)
    , d(new Private(type, hsh, hsl, hih, hil, capabilities))
{}

TabletToolInterface::~TabletToolInterface() = default;

void TabletToolInterface::sendButton(uint32_t serial, uint32_t button, bool pressed)
{
    d->send_button(serial, button, pressed ? QtWaylandServer::zwp_tablet_tool_v2::button_state_pressed : QtWaylandServer::zwp_tablet_tool_v2::button_state_released);
}

void TabletToolInterface::sendMotion(const QPoint& pos)
{
    d->send_motion(pos.x(), pos.y());
}

void TabletToolInterface::sendDistance(uint32_t distance)
{
    d->send_distance(distance);
}

void TabletToolInterface::sendFrame(uint32_t time)
{
    d->send_frame(time);
}

void TabletToolInterface::sendPressure(uint32_t pressure)
{
    d->send_pressure(pressure);
}

void TabletToolInterface::sendRotation(uint32_t degrees)
{
    d->send_rotation(degrees);
}

void TabletToolInterface::sendSlider(int32_t position)
{
    d->send_slider(position);
}

void TabletToolInterface::sendTilt(uint32_t degreesX, uint32_t degreesY)
{
    d->send_tilt(degreesX, degreesY);
}

void TabletToolInterface::sendWheel(int32_t degrees, int32_t clicks)
{
    d->send_wheel(degrees, clicks);
}

void TabletToolInterface::sendProximityIn(uint32_t serial, Resource* tablet, Resource* surface)
{
    d->send_proximity_in(serial, tablet->resource(), surface->resource());
}

void TabletToolInterface::sendProximityOut()
{
    d->send_proximity_out();
}

void TabletToolInterface::sendDown(uint32_t serial)
{
    d->send_down(serial);
}

void TabletToolInterface::sendUp()
{
    d->send_up();
}

void TabletToolInterface::sendRemoved()
{
    d->send_removed();
}

wl_resource * TabletToolInterface::resource() const
{
    return d->resource()->handle;
}

class TabletInterface::Private : public QtWaylandServer::zwp_tablet_v2
{
public:
    Private(uint32_t vendorId, uint32_t productId, const QString name, const QStringList &paths)
        : zwp_tablet_v2()
        , m_vendorId(vendorId)
        , m_productId(productId)
        , m_name(name)
        , m_paths(paths)
    {}

    void zwp_tablet_v2_bind_resource(Resource * /*resource*/) override {
        send_name(m_name);
        send_id(m_vendorId, m_productId);
        for (const auto &path : qAsConst(m_paths))
            send_path(path);
        send_done();
    }

    const uint32_t m_vendorId;
    const uint32_t m_productId;
    const QString m_name;
    const QStringList m_paths;
};

TabletInterface::TabletInterface(uint32_t vendorId, uint32_t productId, const QString &name, const QStringList &paths, QObject* parent)
    : QObject(parent)
    , d(new Private(vendorId, productId, name, paths))
{
}

TabletInterface::~TabletInterface() = default;

wl_resource * TabletInterface::resource() const
{
    return d->resource()->handle;
}

TabletToolInterface * TabletSeatInterface::toolForHardwareId(quint64 serialId) const
{
    for (auto tool : d->m_tools) {
        if (tool->hardwareSerial() == serialId)
            return tool;
    }
    return nullptr;
}

quint64 TabletToolInterface::hardwareSerial() const
{
    return quint64(quint64(d->m_hardwareIdHigh) << 32) + d->m_hardwareIdLow;
}

#if 0

class TabletPadInterface::Private : public QtWaylandServer::zwp_tablet_pad_v2
{
public:
    Private(const QStringList &paths)
        : zwp_tablet_pad_v2()
        , m_paths(paths)
    {}

    void zwp_tablet_pad_v2_bind_resource(Resource * /*resource*/) override {
//         zwp_tablet_pad_group_v2_send_strip();
        for (const auto &path : qAsConst(m_paths))
            send_path(path);
        send_done();
    }
    const QStringList m_paths;
};

TabletPadInterface::TabletPadInterface(const QStringList & paths, QObject* parent)
    : QObject(parent)
    , d(new Private(paths))
{
}

TabletPadInterface::~TabletPadInterface() = default;

void TabletPadInterface::sendEnter(quint32 serial, struct ::wl_resource *tablet, struct ::wl_resource *surface)
{
    d->send_enter(serial, tablet, surface);
}

void TabletPadInterface::sendLeave(quint32 serial, struct ::wl_resource *surface)
{
    d->send_leave(serial, surface);
}

class TabletPadStripInterface::Private : public QtWaylandServer::zwp_tablet_pad_strip_v2
{
public:
    Private(TabletPadStripInterface* q)
        : zwp_tablet_pad_strip_v2()
        , q(q)
    {}

    void zwp_tablet_pad_strip_v2_set_feedback(Resource * resource,
                                              const QString & description,
                                              uint32_t serial) override
    {
        Q_UNUSED(resource);
        Q_UNUSED(serial);
        if (m_description != description) {
            m_description = description;
            Q_EMIT q->descriptionChanged(description);
        }
    }

    QString m_description;
    TabletPadStripInterface* const q;
};

TabletPadStripInterface::TabletPadStripInterface(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
}

TabletPadStripInterface::~TabletPadStripInterface() = default;

QString TabletPadStripInterface::description() const
{
    return d->m_description;
}

void TabletPadStripInterface::sendSource(Source source)
{
    d->send_source(source);
}

void TabletPadStripInterface::sendFrame(quint32 timestamp)
{
    d->send_frame(timestamp);
}

void TabletPadStripInterface::sendPosition(quint32 position)
{
    d->send_position(position);
}

void TabletPadStripInterface::sendStop()
{
    d->send_stop();
}

class TabletPadRingInterface::Private : public QtWaylandServer::zwp_tablet_pad_ring_v2
{
public:
    Private(TabletPadRingInterface* q)
        : zwp_tablet_pad_ring_v2()
        , q(q)
    {}

    void zwp_tablet_pad_ring_v2_set_feedback(Resource * resource,
                                             const QString & description,
                                             uint32_t serial) override
    {
        Q_UNUSED(resource);
        Q_UNUSED(serial);
        if (m_description != description) {
            m_description = description;
            Q_EMIT q->descriptionChanged(description);
        }
    }

    QString m_description;
    TabletPadRingInterface* const q;
};

TabletPadRingInterface::TabletPadRingInterface(QObject* parent)
    : QObject(parent)
    , d(new Private(this))
{
}

TabletPadRingInterface::~TabletPadRingInterface() = default;

void TabletPadRingInterface::sendAngle(quint32 degrees)
{
    d->send_angle(degrees);
}

void TabletPadRingInterface::sendFrame(quint32 time)
{
    d->send_frame(time);
}

void TabletPadRingInterface::sendSource(TabletPadRingInterface::Source source)
{
    d->send_source(source);
}

void TabletPadRingInterface::sendStop()
{
    d->send_stop();
}

class TabletPadGroupInterface::Private : public QtWaylandServer::zwp_tablet_pad_group_v2
{
public:
    Private(TabletPadInterface* pad)
        : zwp_tablet_pad_group_v2()
        , pad(pad)
    {}

    void zwp_tablet_pad_group_v2_bind_resource(Resource * resource) override
    {
        send_buttons({});
        send_done();
    }
    TabletPadInterface* const pad;
};

TabletPadGroupInterface::TabletPadGroupInterface(TabletPadInterface* pad, QObject* parent)
    : QObject(parent)
    , d(new Private(pad))
{
}

TabletPadGroupInterface::~TabletPadGroupInterface() = default;
#endif
