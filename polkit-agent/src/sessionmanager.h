#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QDBusInterface>

class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(QObject *parent = nullptr);

signals:

private slots:
    void onStop();
    void onEndSession(unsigned int flag);
    void onQueryEndSession(unsigned int flag);
    void endSessionResponse();

private:
    QDBusInterface *smInterface;
    QDBusInterface *clientInterface;
    QString clientId;
};

#endif // SESSIONMANAGER_H
