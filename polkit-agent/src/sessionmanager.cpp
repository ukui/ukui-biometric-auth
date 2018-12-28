#include "sessionmanager.h"
#include <QApplication>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QDebug>

#define SM_DBUS_SERVICE "org.gnome.SessionManager"
#define SM_DBUS_PATH "/org/gnome/SessionManager"
#define SM_DBUS_INTERFACE "org.gnome.SessionManager"
#define SM_DBUS_CLIENT_INTERFACE "org.gnome.SessionManager.ClientPrivate"


SessionManager::SessionManager(QObject *parent) : QObject(parent)
{
    smInterface = new QDBusInterface(SM_DBUS_SERVICE,
                             SM_DBUS_PATH,
                             SM_DBUS_INTERFACE,
                             QDBusConnection::sessionBus());
    QString appId("polkit-ukui-authentication-agent-1.desktop");
    QString clientStartupId(qgetenv("DESKTOP_AUTOSTART_ID"));

    QDBusReply<QDBusObjectPath> reply = smInterface->call("RegisterClient",
                                                       appId, clientStartupId);

    if(!reply.isValid()) {
        qWarning() << "Register Client to gnome session failed";
    }
    clientId = reply.value().path();

    qDebug() << "Register Client to gnome session: " << reply.value().path();

    clientInterface = new QDBusInterface(SM_DBUS_SERVICE,
                                   clientId,
                                   SM_DBUS_CLIENT_INTERFACE,
                                   QDBusConnection::sessionBus());
    qDebug() << clientInterface->isValid();

    QDBusConnection conn = clientInterface->connection();
    conn.connect(SM_DBUS_SERVICE, clientId, SM_DBUS_CLIENT_INTERFACE,
                 "Stop", this, SLOT(onStop()));
    conn.connect(SM_DBUS_SERVICE, clientId, SM_DBUS_CLIENT_INTERFACE,
                 "EndSession", this, SLOT(onEndSession(unsigned int)));
    conn.connect(SM_DBUS_SERVICE, clientId, SM_DBUS_CLIENT_INTERFACE,
                 "QueryEndSession", this, SLOT(onQueryEndSession(unsigned int)));

}

void SessionManager::onStop()
{
    QApplication::quit();
}

void SessionManager::onEndSession(unsigned int flag)
{
    endSessionResponse();
}

void SessionManager::onQueryEndSession(unsigned int flag)
{
    endSessionResponse();
    QApplication::quit();
}

void SessionManager::endSessionResponse()
{
    QDBusMessage msg = clientInterface->call("EndSessionResponse", true, "");
    if(msg.type() == QDBusMessage::ErrorMessage)
        qDebug() << "Failed to call EndSessionResponse " << msg.errorMessage();
}
