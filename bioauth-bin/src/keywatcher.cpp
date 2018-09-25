#include "keywatcher.h"
#include <termios.h>

KeyWatcher::KeyWatcher(QObject *parent)
    : QThread(parent)
{
}

void KeyWatcher::run()
{
    // 修改终端属性，保证按键被立即接收
    struct termios current;
    tcgetattr(0, &save);
    current = save;
    current.c_lflag &= ~ICANON;
    current.c_lflag &= ~ECHO;
    current.c_cc[VMIN] = 1;
    current.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &current);

    while(!isInterruptionRequested()){

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);

        char ch;
        switch(select(32, &readfds, NULL, NULL, NULL)){
        case 0:
        //	printf("select time out\n");
            break;
        case -1:
        //	printf("select error\n");
            break;
        default:
            // 'q' | 'Q' or Esc
            if((ch = getchar()) == 'q' || ch == 'Q' || ch == 27){
                tcsetattr(0, TCSANOW, &save);
                emit exit();
            }
        }
    }

    tcsetattr(0, TCSANOW, &save);   // 恢复原来的终端属性，以免干扰shall和之后的程序运行
}

void KeyWatcher::stop()
{
    tcsetattr(0, TCSANOW, &save);

    requestInterruption();
    terminate();
    wait();
}
