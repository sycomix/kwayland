/****************************************************************************
Copyright 2018  Marco Martin <notmart@gmail.com>

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
****************************************************************************/
#include "plasmavirtualdesktop_interface.h"
#include "display.h"
#include "global_p.h"
#include "resource_p.h"

#include <QDebug>
#include <QTimer>

#include <wayland-server.h>
#include <wayland-org_kde_plasma_virtual_desktop-server-protocol.h>

namespace KWayland
{
namespace Server
{

class PlasmaVirtualDesktopInterface::Private
{
public:
    Private(PlasmaVirtualDesktopInterface *q, PlasmaVirtualDesktopManagementInterface *c);
    ~Private();
    void createResource(wl_resource *parent, quint32 serial);

    void setLayoutPosition(quint32 row, quint32 column);

    PlasmaVirtualDesktopInterface *q;
    PlasmaVirtualDesktopManagementInterface *vdm;

    QVector<wl_resource*> resources;
    QString id;
    QString name;
    quint32 row = 0;
    quint32 column = 0;
    bool active = false;

private:
    static void unbind(wl_resource *resource);
    static void requestActivateCallback(wl_client *client, wl_resource *resource);

    static Private *cast(wl_resource *resource) {
        return reinterpret_cast<Private*>(wl_resource_get_user_data(resource));
    }

    wl_listener listener;
    static const struct org_kde_plasma_virtual_desktop_interface s_interface;
};


class PlasmaVirtualDesktopManagementInterface::Private : public Global::Private
{
public:
    Private(PlasmaVirtualDesktopManagementInterface *q, Display *d);

    /**
     * holeRow and holeColumn are used to leave an empty spot in
     * the desktop numeration
     * use -1 to not leave any hole
     */
    void sortDesktops(quint32 holeRow, quint32 holeColumn);
    void updateColumns();

    QVector<wl_resource*> resources;
    QMap<QString, PlasmaVirtualDesktopInterface *> desktops;
    QList<PlasmaVirtualDesktopInterface *> orderedDesktops;
    //can't be less than 1 row
    quint32 rows = 1;
    quint32 columns = 0;

private:
    void bind(wl_client *client, uint32_t version, uint32_t id) override;

    static void unbind(wl_resource *resource);
    static Private *cast(wl_resource *r) {
        return reinterpret_cast<Private*>(wl_resource_get_user_data(r));
    }

    static void getVirtualDesktopCallback(wl_client *client, wl_resource *resource, uint32_t serial, const char *id);
    static void releaseCallback(wl_client *client, wl_resource *resource);

    PlasmaVirtualDesktopManagementInterface *q;

    static const struct org_kde_plasma_virtual_desktop_management_interface s_interface;
    static const quint32 s_version;
};

const quint32 PlasmaVirtualDesktopManagementInterface::Private::s_version = 1;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
const struct org_kde_plasma_virtual_desktop_management_interface PlasmaVirtualDesktopManagementInterface::Private::s_interface = {
    getVirtualDesktopCallback,
    releaseCallback
};
#endif

void PlasmaVirtualDesktopManagementInterface::Private::getVirtualDesktopCallback(wl_client *client, wl_resource *resource, uint32_t serial, const char *id)
{
    Q_UNUSED(client)
    auto s = cast(resource);

    auto i = s->desktops.constFind(QString::fromUtf8(id));
    if (i == s->desktops.constEnd()) {
        return;
    }

    (*i)->d->createResource(resource, serial);
}

void PlasmaVirtualDesktopManagementInterface::Private::releaseCallback(wl_client *client, wl_resource *resource)
{
    Q_UNUSED(client)
    wl_resource_destroy(resource);
}

void PlasmaVirtualDesktopManagementInterface::Private::sortDesktops(quint32 holeRow, quint32 holeColumn)
{
    qSort(orderedDesktops.begin(), orderedDesktops.end(), [](PlasmaVirtualDesktopInterface *d1, PlasmaVirtualDesktopInterface *d2) {
            if (d2->row() > d1->row()) {
                return true;
            } else if (d2->row() == d1->row()) {
                return d2->column() > d1->column();
            } else {
                return false;
            }
        });

        const quint32 usedColumns = qMax(1.0, ceil((qreal)desktops.count() / (qreal)rows));
        int i = 0;
        for (const auto desktop : orderedDesktops) {
            quint32 newRow = floor(i / usedColumns);
            quint32 newColumn = (i % usedColumns);
            //skip a cell if we're asked to
            if (newRow == holeRow && newColumn == holeColumn) {
                ++i;
                newRow = floor(i / usedColumns);
                newColumn = (i % usedColumns);
            }

            desktop->d->setLayoutPosition(newRow, newColumn);

            ++i;
        }
}

void PlasmaVirtualDesktopManagementInterface::Private::updateColumns()
{
    const quint32 newColumns = ceil((qreal)desktops.count() / (qreal)rows);

    if (columns != newColumns) {
        columns = newColumns;

        for (auto it = resources.constBegin(); it != resources.constEnd(); ++it) {
            org_kde_plasma_virtual_desktop_management_send_layout(*it, rows, columns);
        }
        sortDesktops(-1, -1);
    }
}

PlasmaVirtualDesktopManagementInterface::Private::Private(PlasmaVirtualDesktopManagementInterface *q, Display *d)
    : Global::Private(d, &org_kde_plasma_virtual_desktop_management_interface, s_version)
    , q(q)
{
}

void PlasmaVirtualDesktopManagementInterface::Private::bind(wl_client *client, uint32_t version, uint32_t id)
{
    auto c = display->getConnection(client);
    wl_resource *resource = c->createResource(&org_kde_plasma_virtual_desktop_management_interface, qMin(version, s_version), id);

    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    resources << resource;

    wl_resource_set_implementation(resource, &s_interface, this, unbind);

    for (auto it = desktops.constBegin(); it != desktops.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_management_send_desktop_added(resource, (*it)->id().toUtf8().constData());
    }
    org_kde_plasma_virtual_desktop_management_send_layout(resource, rows, columns);
}

void PlasmaVirtualDesktopManagementInterface::Private::unbind(wl_resource *resource)
{
    auto dm = reinterpret_cast<Private*>(wl_resource_get_user_data(resource));
    dm->resources.removeAll(resource);
}

PlasmaVirtualDesktopManagementInterface::PlasmaVirtualDesktopManagementInterface(Display *display, QObject *parent)
    : Global(new Private(this, display), parent)
{
}

PlasmaVirtualDesktopManagementInterface::~PlasmaVirtualDesktopManagementInterface()
{}

PlasmaVirtualDesktopManagementInterface::Private *PlasmaVirtualDesktopManagementInterface::d_func() const
{
    return reinterpret_cast<Private*>(d.data());
}

void PlasmaVirtualDesktopManagementInterface::setRows(quint32 rows)
{
    Q_D();
    if (rows == 0 || d->rows == rows) {
        return;
    }

    d->rows = rows;
    d->columns = ceil((qreal)d->desktops.count() / (qreal)rows);

    for (auto it = d->resources.constBegin(); it != d->resources.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_management_send_layout(*it, d->rows, d->columns);
    }
}

quint32 PlasmaVirtualDesktopManagementInterface::rows()
{
    Q_D();
    return d->rows;
}

quint32 PlasmaVirtualDesktopManagementInterface::columns()
{
    Q_D();
    return d->columns;
}

PlasmaVirtualDesktopInterface *PlasmaVirtualDesktopManagementInterface::desktop(const QString &id)
{
    Q_D();
    
    auto i = d->desktops.constFind(id);
    if (i != d->desktops.constEnd()) {
        return *i;
    }
    return nullptr;
}

PlasmaVirtualDesktopInterface *PlasmaVirtualDesktopManagementInterface::createDesktop(const QString &id)
{
    Q_D();
    
    auto i = d->desktops.constFind(id);
    if (i != d->desktops.constEnd()) {
        return *i;
    }

    PlasmaVirtualDesktopInterface *desktop = new PlasmaVirtualDesktopInterface(this);
    desktop->d->id = id;
    for (auto it = desktop->d->resources.constBegin(); it != desktop->d->resources.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_send_id(*it, id.toUtf8().constData());
    }

    //activate the first desktop TODO: to be done here?
    if (d->desktops.isEmpty()) {
        desktop->d->active = true;
    }

    //decide default positioning
    const quint32 columns = qMax(1.0, ceil(d->desktops.count() / d->rows));
    desktop->d->row = floor(d->desktops.count() / columns);
    desktop->d->column = d->desktops.count() % columns;

    d->desktops[id] = desktop;
    d->orderedDesktops << desktop;

    connect(desktop, &QObject::destroyed, this,
        [this, id] {
            Q_D();
            d->desktops.remove(id);
            //TODO: activate another desktop?
            d->updateColumns();
            d->sortDesktops(-1, -1);
        }
    );

    for (auto it = d->resources.constBegin(); it != d->resources.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_management_send_desktop_added(*it, id.toUtf8().constData());
    }

    d->updateColumns();

    return desktop;
}

void PlasmaVirtualDesktopManagementInterface::removeDesktop(const QString &id)
{
    Q_D();
    
    auto deskIt = d->desktops.constFind(id);
    if (deskIt == d->desktops.constEnd()) {
        return;
    }

    for (auto it = (*deskIt)->d->resources.constBegin(); it != (*deskIt)->d->resources.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_send_removed(*it);
    }

    for (auto it = d->resources.constBegin(); it != d->resources.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_management_send_desktop_removed(*it, id.toUtf8().constData());
    }

    d->orderedDesktops.removeAll(*deskIt);

    (*deskIt)->deleteLater();
}

QList <PlasmaVirtualDesktopInterface *> PlasmaVirtualDesktopManagementInterface::desktops() const
{
    Q_D();
    return d->desktops.values();
}

void PlasmaVirtualDesktopManagementInterface::sendDone()
{
    Q_D();
    for (auto it = d->resources.constBegin(); it != d->resources.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_management_send_done(*it);
    }
}

void PlasmaVirtualDesktopManagementInterface::setActiveDesktop(const QString &id)
{
    Q_D();
    for (auto it = d->desktops.constBegin(); it != d->desktops.constEnd(); ++it) {
        auto desktop = *it;
        if (desktop->id() == id) {
            desktop->d->active = true;
            for (auto it = desktop->d->resources.constBegin(); it != desktop->d->resources.constEnd(); ++it) {
                org_kde_plasma_virtual_desktop_send_activated(*it);
            }
        } else {
            if (desktop->d->active) {
                desktop->d->active = false;
                for (auto it = desktop->d->resources.constBegin(); it != desktop->d->resources.constEnd(); ++it) {
                    org_kde_plasma_virtual_desktop_send_deactivated(*it);
                }
            }
        }
    }
}



//// PlasmaVirtualDesktopInterface

#ifndef DOXYGEN_SHOULD_SKIP_THIS
const struct org_kde_plasma_virtual_desktop_interface PlasmaVirtualDesktopInterface::Private::s_interface = {
    requestActivateCallback
};
#endif

void PlasmaVirtualDesktopInterface::Private::requestActivateCallback(wl_client *client, wl_resource *resource)
{
    Q_UNUSED(client)
    auto s = cast(resource);
    emit s->q->activateRequested();
}

PlasmaVirtualDesktopInterface::Private::Private(PlasmaVirtualDesktopInterface *q, PlasmaVirtualDesktopManagementInterface *c)
    : q(q),
      vdm(c)
{
}

PlasmaVirtualDesktopInterface::Private::~Private()
{
   // need to copy, as destroy goes through the destroy listener and modifies the list as we iterate
    const auto c = resources;
    for (const auto &r : c) {
        auto client = wl_resource_get_client(r);
        org_kde_plasma_virtual_desktop_send_removed(r);
        wl_resource_destroy(r);
        wl_client_flush(client);
    }
}

void PlasmaVirtualDesktopInterface::Private::unbind(wl_resource *resource)
{
    Private *p = reinterpret_cast<Private*>(wl_resource_get_user_data(resource));
    p->resources.removeAll(resource);
    if (p->resources.isEmpty()) {
        p->q->deleteLater();
    }
}

void PlasmaVirtualDesktopInterface::Private::createResource(wl_resource *parent, quint32 serial)
{
    ClientConnection *c = vdm->display()->getConnection(wl_resource_get_client(parent));
    wl_resource *resource = c->createResource(&org_kde_plasma_virtual_desktop_interface, wl_resource_get_version(parent), serial);
    if (!resource) {
        return;
    }
    wl_resource_set_implementation(resource, &s_interface, this, unbind);
    resources << resource;

    org_kde_plasma_virtual_desktop_send_id(resource, id.toUtf8().constData());
    if (!name.isEmpty()) {
        org_kde_plasma_virtual_desktop_send_name(resource, name.toUtf8().constData());
    }
    org_kde_plasma_virtual_desktop_send_layout_position(resource, row, column);

    if (active) {
        org_kde_plasma_virtual_desktop_send_activated(resource);
    }

    c->flush();
}

void PlasmaVirtualDesktopInterface::Private::setLayoutPosition(quint32 newRow, quint32 newColumn)
{
    if (row == newRow && column == newColumn) {
        return;
    }

    row = newRow;
    column = newColumn;

    for (auto it = resources.constBegin(); it != resources.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_send_layout_position(*it, row, column);
    }
}

PlasmaVirtualDesktopInterface::PlasmaVirtualDesktopInterface(PlasmaVirtualDesktopManagementInterface *parent)
    : QObject(parent),
      d(new Private(this, parent))
{
}

PlasmaVirtualDesktopInterface::~PlasmaVirtualDesktopInterface()
{}

QString PlasmaVirtualDesktopInterface::id() const
{
    return d->id;
}

void PlasmaVirtualDesktopInterface::setName(const QString &name)
{
    if (d->name == name) {
        return;
    }

    d->name = name;
    for (auto it = d->resources.constBegin(); it != d->resources.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_send_name(*it, name.toUtf8().constData());
    }
}

QString PlasmaVirtualDesktopInterface::name() const
{
    return d->name;
}

void PlasmaVirtualDesktopInterface::setLayoutPosition(quint32 row, quint32 column)
{
    if (d->row == row && d->column == column) {
        return;
    }

    //make sure this desktop will be the last
    d->row = d->vdm->rows();
    d->column = d->vdm->columns();
    //sort desktops with this new position as an "hole"
    d->vdm->d_func()->sortDesktops(row, column);
    //finally put this desktop in the desired position
    d->setLayoutPosition(row, column);
}

quint32 PlasmaVirtualDesktopInterface::row() const
{
    return d->row;
}

quint32 PlasmaVirtualDesktopInterface::column() const
{
    return d->column;
}

bool PlasmaVirtualDesktopInterface::active() const
{
    return d->active;
}

void PlasmaVirtualDesktopInterface::sendDone()
{
    for (auto it = d->resources.constBegin(); it != d->resources.constEnd(); ++it) {
        org_kde_plasma_virtual_desktop_send_done(*it);
    }
}

}
}

