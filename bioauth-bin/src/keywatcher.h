#ifndef KEYWATCHER_H
#define KEYWATCHER_H

#include <QThread>
#include <termios.h>

class KeyWatcher : public QThread
{
    Q_OBJECT
public:
    explicit KeyWatcher(QObject *parent = nullptr);
    void run() override;
    void stop();

signals:
    void exit();

private:
    struct termios save;
};

#endif // KEYWATCHER_H
