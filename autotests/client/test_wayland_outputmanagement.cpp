/********************************************************************
Copyright 2014 Martin Gräßlin <mgraesslin@kde.org>
Copyright 2015 Sebastian Kügler <sebas@kde.org>

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
// Qt
#include <QtTest/QtTest>
// KWin
#include "../../src/client/connection_thread.h"
#include "../../src/client/event_queue.h"
#include "../../src/client/outputdevice.h"
#include "../../src/client/outputconfiguration.h"
#include "../../src/client/outputmanagement.h"
#include "../../src/client/output.h"
#include "../../src/client/registry.h"
#include "../../src/server/display.h"
#include "../../src/server/shell_interface.h"
#include "../../src/server/compositor_interface.h"
#include "../../src/server/outputconfiguration_interface.h"
#include "../../src/server/outputdevice_interface.h"
#include "../../src/server/outputmanagement_interface.h"

// Wayland
#include <wayland-client-protocol.h>

class TestWaylandOutputManagement : public QObject
{
    Q_OBJECT
public:
    explicit TestWaylandOutputManagement(QObject *parent = nullptr);
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();


    void testCreate();
    void testOutputDevices();
    void createConfig();

    void testEnable();
    void testRemoval();

private:


    KWayland::Server::Display *m_display;
    KWayland::Server::OutputConfigurationInterface *m_outputConfigurationInterface;
    KWayland::Server::OutputManagementInterface *m_outputManagementInterface;
    QList<KWayland::Server::OutputDeviceInterface *> m_serverOutputs;


    KWayland::Client::Registry m_registry;
    KWayland::Client::OutputDevice *m_outputDevice;
    KWayland::Client::OutputManagement m_outputManagement;
    KWayland::Client::OutputConfiguration *m_outputConfiguration;
    QList<KWayland::Client::OutputDevice *> m_clientOutputs;

    KWayland::Client::ConnectionThread *m_connection;
    KWayland::Client::EventQueue *m_queue;
    QThread *m_thread;

    QSignalSpy *m_announcedSpy;
    QSignalSpy *m_omSpy;
};

static const QString s_socketName = QStringLiteral("kwin-test-wayland-output-0");

TestWaylandOutputManagement::TestWaylandOutputManagement(QObject *parent)
    : QObject(parent)
    , m_display(nullptr)
    , m_outputConfigurationInterface(nullptr)
    , m_outputManagementInterface(nullptr)
    , m_connection(nullptr)
    , m_queue(nullptr)
    , m_thread(nullptr)
    , m_announcedSpy(nullptr)
{
}

void TestWaylandOutputManagement::initTestCase()
{
    using namespace KWayland::Server;
    delete m_display;
    m_display = new Display(this);
    m_display->setSocketName(s_socketName);
    m_display->start();
    QVERIFY(m_display->isRunning());

    auto shell = m_display->createShell(this);
    shell->create();
    auto comp = m_display->createCompositor(this);
    comp->create();

    auto outputDeviceInterface = m_display->createOutputDevice(this);
    outputDeviceInterface->addMode(QSize(800, 600), OutputDeviceInterface::ModeFlags(OutputDeviceInterface::ModeFlag::Preferred));
    outputDeviceInterface->addMode(QSize(1024, 768));
    outputDeviceInterface->addMode(QSize(1280, 1024), OutputDeviceInterface::ModeFlags(), 90000);
    outputDeviceInterface->setCurrentMode(QSize(1024, 768));
    outputDeviceInterface->setGlobalPosition(QPoint(0, 1920));
    outputDeviceInterface->create();
    m_serverOutputs << outputDeviceInterface;

    m_outputManagementInterface = m_display->createOutputManagement(this);
    m_outputManagementInterface->create();
    QVERIFY(m_outputManagementInterface->isValid());

    // setup connection
    m_connection = new KWayland::Client::ConnectionThread;
    QSignalSpy connectedSpy(m_connection, SIGNAL(connected()));
    m_connection->setSocketName(s_socketName);

    m_thread = new QThread(this);
    m_connection->moveToThread(m_thread);
    m_thread->start();

    m_connection->initConnection();
    QVERIFY(connectedSpy.wait());

    m_queue = new KWayland::Client::EventQueue(this);
    QVERIFY(!m_queue->isValid());
    m_queue->setup(m_connection);
    QVERIFY(m_queue->isValid());
}

void TestWaylandOutputManagement::cleanupTestCase()
{
    if (m_queue) {
        delete m_queue;
        m_queue = nullptr;
    }
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;
    }
    delete m_connection;
    m_connection = nullptr;

    delete m_display;
    m_display = nullptr;
}

void TestWaylandOutputManagement::testCreate()
{
    m_announcedSpy = new QSignalSpy(&m_registry, &KWayland::Client::Registry::outputManagementAnnounced);
    m_omSpy = new QSignalSpy(&m_registry, SIGNAL(outputDeviceAnnounced(quint32,quint32)));

    QVERIFY(m_announcedSpy->isValid());
    QVERIFY(m_omSpy->isValid());

    m_registry.create(m_connection->display());
    QVERIFY(m_registry.isValid());
    m_registry.setup();
    wl_display_flush(m_connection->display());

    QVERIFY(m_announcedSpy->wait());
    QCOMPARE(m_announcedSpy->count(), 1);

    m_outputManagement.setup(m_registry.bindOutputManagement(m_announcedSpy->first().first().value<quint32>(), m_announcedSpy->first().last().value<quint32>()));
}

void TestWaylandOutputManagement::testOutputDevices()
{

    //m_display->createOutputDevice()->create();
    //QVERIFY(m_omSpy->wait());
    QCOMPARE(m_omSpy->count(), 1);
    QCOMPARE(m_registry.interfaces(KWayland::Client::Registry::Interface::OutputDevice).count(), m_serverOutputs.count());


    auto output = new KWayland::Client::OutputDevice();
    QVERIFY(!output->isValid());
    QCOMPARE(output->geometry(), QRect());
    QCOMPARE(output->globalPosition(), QPoint());
    QCOMPARE(output->manufacturer(), QString());
    QCOMPARE(output->model(), QString());
    QCOMPARE(output->physicalSize(), QSize());
    QCOMPARE(output->pixelSize(), QSize());
    QCOMPARE(output->refreshRate(), 0);
    QCOMPARE(output->scale(), 1);
    QCOMPARE(output->subPixel(), KWayland::Client::OutputDevice::SubPixel::Unknown);
    QCOMPARE(output->transform(), KWayland::Client::OutputDevice::Transform::Normal);
    QCOMPARE(output->enabled(), true);
    QCOMPARE(output->edid()->eisaId, QString());

    QSignalSpy outputChanged(output, &KWayland::Client::OutputDevice::changed);
    QVERIFY(outputChanged.isValid());

    output->setup(m_registry.bindOutputDevice(m_omSpy->first().first().value<quint32>(), m_omSpy->first().last().value<quint32>()));
    wl_display_flush(m_connection->display());

    QVERIFY(outputChanged.wait());
    //KWayland::Client::OutputManagement outputmanagement;
    //    wl_display_flush(m_connection->display());
    QCOMPARE(output->globalPosition(), QPoint(0,1920));
    QCOMPARE(output->enabled(), true);

    m_clientOutputs << output;
    m_outputDevice = output;

    QVERIFY(m_outputManagement.isValid());
}

void TestWaylandOutputManagement::testRemoval()
{
    QSignalSpy outputManagementRemovedSpy(&m_registry,&KWayland::Client::Registry::outputManagementRemoved);
    QVERIFY(outputManagementRemovedSpy.isValid());

    delete m_outputManagementInterface;
    QVERIFY(outputManagementRemovedSpy.wait());
    QCOMPARE(outputManagementRemovedSpy.first().first(), m_announcedSpy->first().first());
    QVERIFY(!m_registry.hasInterface(KWayland::Client::Registry::Interface::OutputManagement));
    QVERIFY(m_registry.interfaces(KWayland::Client::Registry::Interface::OutputManagement).isEmpty());
}

void TestWaylandOutputManagement::createConfig()
{
    m_outputConfiguration = m_outputManagement.createConfiguration();
    //m_outputConfiguration->setEnabled(0, 0);
    QVERIFY(m_outputConfiguration->isValid());

    //config->enable(m_serverOutput, false);


    //config->setScale(m_serverOutput, 2);
    //config->setPosition(m_serverOutput, QPoint(0, 1920); // right of full HD display
    //config->setMode(m_serverOutput, 0);
    //config->setTransform(m_serverOutput, 0);

    //QSignalSpy enabledChangedSpy(&m_serverOutput, SIGNAL(enabledChanged()));

    //config->apply();
    //QVERIFY(enabledChangedSpy.wait(1000));
}

void TestWaylandOutputManagement::testEnable()
{
    auto config = m_outputConfiguration;

    QVERIFY(config->isValid());
    //m_registry.interfaces(KWayland::Client::Registry::Interface::OutputDevice)
    KWayland::Client::OutputDevice *output = m_clientOutputs.first();
    QCOMPARE(output->enabled(), true);

    QSignalSpy enabledChanged(output, SIGNAL(changed()));
    QVERIFY(enabledChanged.isValid());

    //m_serverOutput->setEnabled(false);
    ////wl_display_flush(m_connection->display());

    config->setEnabled(output, false);


    QVERIFY(enabledChanged.wait(200));
    QCOMPARE(output->enabled(), false);

    m_serverOutputs.first()->setEnabled(true);

    QVERIFY(enabledChanged.wait(200));
    QCOMPARE(output->enabled(), true);

}


QTEST_GUILESS_MAIN(TestWaylandOutputManagement)
#include "test_wayland_outputmanagement.moc"
