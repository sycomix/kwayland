/********************************************************************
Copyright 2015  Martin Gräßlin <mgraesslin@kde.org>

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
#ifndef KWAYLAND_SERVER_IDLE_INTERFACE_H
#define KWAYLAND_SERVER_IDLE_INTERFACE_H

#include <KWayland/Server/kwaylandserver_export.h>
#include "global.h"
#include "resource.h"

namespace KWayland
{
namespace Server
{

class Display;
class SeatInterface;

class KWAYLANDSERVER_EXPORT IdleInterface : public Global
{
    Q_OBJECT
public:
    virtual ~IdleInterface();

private:
    explicit IdleInterface(Display *display, QObject *parent = nullptr);
    friend class Display;
    class Private;
};

class KWAYLANDSERVER_EXPORT IdleTimeoutInterface : public Resource
{
    Q_OBJECT
public:
    virtual ~IdleTimeoutInterface();

private:
    explicit IdleTimeoutInterface(SeatInterface *seat, IdleInterface *parent, wl_resource *parentResource);
    friend class IdleInterface;
    class Private;
    Private *d_func() const;
};

}
}

#endif
