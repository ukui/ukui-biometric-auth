/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
**/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMovie>
#include <QFile>
#include <QKeyEvent>
#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QFontMetrics>
#include <QtMath>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <libintl.h>
#include <locale.h>
#include "bioauthwidget.h"
#include <QHBoxLayout>
#include <QAction>
#include "generic.h"
#define _(string) gettext(string)
extern void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed);
MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow),
    users(new Users(this)),
    enableBioAuth(false),
    receiveBioPAM(false),
    authMode(UNDEFINED),
    useDoubleAuth(false),
    m_timer(nullptr),
    isLockingFlg(false),
    isbioSuccess(false)
{
    ui->setupUi(this);
    setWindowTitle(tr("Authentication"));
    setWindowFlags(Qt::WindowStaysOnTopHint);

    pam_tally_init(); //

    widgetBioAuth = new BioAuthWidget(this);
    widgetBioDevices = new BioDevicesWidget(this);
    //ui->formLayout->addWidget(widgetBioAuth);
    //ui->formLayout->addWidget(widgetBioDevices);

    ui->bioAuthLayout->addWidget(widgetBioAuth);
    ui->bioAuthLayout->addWidget(widgetBioDevices);

    maxFailedTimes = bioDevices.getFailedTimes();
    isHiddenSwitchButton = bioDevices.GetHiddenSwitchButton();

    if(isHiddenSwitchButton)
        widgetBioAuth->hidePasswdButton();

    connect(widgetBioDevices, &BioDevicesWidget::deviceChanged,
            this, [&](const DeviceInfo &device){
        widgetBioAuth->startAuth(getUid(userName), device);
        switchWidget(BIOMETRIC);
    });

    connect(widgetBioDevices, &BioDevicesWidget::back,
            this, [&]{
        switchWidget(BIOMETRIC);
    });

    connect(&bioDevices, &BioDevices::deviceCountChanged,
            this, [&]{
        widgetBioAuth->setMoreDevices(bioDevices.GetUserDevCount(getUid(userName)) > 1);
    });

    connect(widgetBioAuth, &BioAuthWidget::authComplete,
            this, [&](uid_t uid, bool ret){
        qDebug() << "biometric authentication complete: " << uid << ret;

        if(uid == getUid(userName) && ret)
        {
            if(useDoubleAuth){
                isbioSuccess = true;
                emit switchToBiometric();
                authMode = UNDEFINED;
            }else{
                accept(BIOMETRIC_SUCCESS);
            }
        }else if(useDoubleAuth){
            if(m_failMap.contains(getUid(userName))){
                m_failMap[getUid(userName)] = m_failMap[getUid(userName)] + 1;
            }else{
                m_failMap[getUid(userName)] = 1;
            }
            if(m_failMap[getUid(userName)] >= maxFailedTimes){
                setMessage(tr("Too many unsuccessful attempts,please enter password."));
                return ;
            }

            setMessage(tr("Fingerprint authentication failed, you still have %1 verification opportunities")\
                          .arg(maxFailedTimes - m_failMap[getUid(userName)]));
           // widgetBioAuth->startAuth(getUid(userName), *device);
            if(!isbioSuccess){
                QTimer::singleShot(1000,this,SLOT(restart_bio_identify()));
            }
        }
    });

    connect(widgetBioAuth, &BioAuthWidget::selectDevice,
            this, [&]{
        //widgetBioDevices->init(getUid(userName));
        switchWidget(DEVICES);
    });

    connect(widgetBioAuth, &BioAuthWidget::switchToPassword,
            this, [&] {
        accept(BIOMETRIC_IGNORE);
    });

    QFile qssFile(":/qss/src/main.qss");
    qssFile.open(QIODevice::ReadOnly);
    setStyleSheet(qssFile.readAll());
    qssFile.close();

    //ui->cmbUsers->hide();
    //ui->widgetDetails->hide();
    //ui->btnDetails->setIcon(QIcon(":/image/assets/arrow_right.svg"));
    //ui->btnDetails->hide();
    //ui->lePassword->installEventFilter(this);
    switchWidget(UNDEFINED);
    editIcon();
    ui->lblContent->adjustSize();
    ui->lblContent->height();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::restart_bio_identify()
{
    DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
    if(device){
        widgetBioAuth->startAuth(getUid(userName), *device);
        setMessage(tr("Please enter your password or enroll your fingerprint "));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    widgetBioAuth->stopAuth();
    emit canceled();

    return QWidget::closeEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event)
{

    Q_UNUSED(event);
    QStyleOption *option = new QStyleOption();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath rectPath;
    rectPath.addRoundedRect(this->rect().adjusted(0,0,0,0),0,0);
    // 画一个黑底
    QPixmap pixmap(this->rect().size());
    pixmap.fill(Qt::transparent);
    QPainter pixmapPainter(&pixmap);
    pixmapPainter.setRenderHint(QPainter::Antialiasing);
    pixmapPainter.setPen(Qt::transparent);
    pixmapPainter.setBrush(QColor(0,0,0,100));
    pixmapPainter.drawPath(rectPath);
    pixmapPainter.end();

    // 模糊这个黑底
    QImage img = pixmap.toImage();
    qt_blurImage(img, 10, false, false);

    // 挖掉中心
    pixmap = QPixmap::fromImage(img);
    QPainter pixmapPainter2(&pixmap);
    pixmapPainter2.setRenderHint(QPainter::Antialiasing);
    pixmapPainter2.setCompositionMode(QPainter::CompositionMode_Clear);
    pixmapPainter2.setPen(Qt::transparent);
    pixmapPainter2.setBrush(Qt::transparent);
    pixmapPainter2.drawPath(rectPath);

    // 绘制阴影
    p.drawPixmap(this->rect(), pixmap, pixmap.rect());

    // 绘制一个背景
    p.save();
    p.fillPath(rectPath,option->palette.color(QPalette::Base));
    p.restore();
    //绘制一个矩形

    p.setPen(Qt::red);
    QRectF rect(0,290,20,20);

}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == ui->lePassword){
        if(event->type() == QEvent::KeyPress){ //禁止复制粘贴功能。
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if(keyEvent->matches(QKeySequence::Copy) || keyEvent->matches(QKeySequence::Cut) || keyEvent->matches(QKeySequence::Paste)){
                event->ignore();
                return true;
            }
        }
    }
    return false;
}

/*** main ***/
void MainWindow::on_cmbUsers_currentTextChanged(const QString &userName)
{
    qDebug() << "user changed to " << userName;
    widgetBioAuth->stopAuth();	
    authMode=UNDEFINED;

    QList<UserItem> accountUsers = users->getUsers();
    this->userName = userName;
    for(UserItem user:accountUsers){
        if(user.realName == userName){
            this->userName = user.name;
        }
    }

    ui->lblMessage->clear();
    widgetBioDevices->init(getUid(this->userName));
    emit userChanged(this->userName);
}

int MainWindow::enable_biometric_authentication()
{
    char conf_file[] = GET_STR(CONFIG_FILE);
    FILE *file;
    char line[1024], is_enable[16];
    int i;


    if((file = fopen(conf_file, "r")) == NULL){
        return 0;
    }
    while(fgets(line, sizeof(line), file)) {
        i = sscanf(line, "EnableAuth=%s\n",  is_enable);
        if(i > 0) {
            break;
        }
    }

    fclose(file);
    if(!strcmp(is_enable, "true"))
        return 1;
    return 0;
}

void MainWindow::on_btnDetails_clicked()
{/*
    if(ui->widgetDetails->isHidden()) {
        ui->widgetDetails->show();
        ui->btnDetails->setIcon(QIcon(":/image/assets/arrow_down.svg"));
//        resize(width(), height() + ui->widgetDetails->height());
    }
    else {
        ui->widgetDetails->hide();
        ui->btnDetails->setIcon(QIcon(":/image/assets/arrow_right.svg"));
//        resize(width(), height() - ui->widgetDetails->height());
    }
    adjustSize();*/
}

void MainWindow::on_btnCancel_clicked()
{
    close();
}

void MainWindow::on_btnAuth_clicked()
{
    on_lePassword_returnPressed();
}

/*** pagePassword ***/
void MainWindow::editIcon()
{
    m_modeButton = new QPushButton(ui->lePassword);
    m_modeButton->setObjectName(QStringLiteral("echoModeButton"));
    m_modeButton->setCheckable(true);
    m_modeButton->setFocusPolicy(Qt::NoFocus);
    m_modeButton->setCursor(Qt::PointingHandCursor);
    connect(m_modeButton, &QPushButton::clicked, this, [&](bool checked){
        setType(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
    QHBoxLayout *layout = new QHBoxLayout(ui->lePassword);
    //layout->setSpacing(0);
    layout->addStretch();
    layout->addWidget(m_modeButton);
    layout->setContentsMargins(0, 0, 0, 0);


}

void MainWindow::setType(QLineEdit::EchoMode type)
{
    ui->lePassword->setEchoMode(type);
    if(type == 0)
       m_modeButton->setChecked(true);
    else
       m_modeButton->setChecked(false);
}

void MainWindow::on_lePassword_returnPressed()
{
    emit accept(ui->lePassword->text());
    ui->btnAuth->hide();
    // switchWidget(UNDEFINED);
    // setMessage(tr("in authentication, please wait..."));
}

void MainWindow::on_btnBioAuth_clicked()
{
    emit switchToBiometric();
    authMode = BIOMETRIC;
}

void MainWindow::on_returnButton_clicked()
{

    widgetBioAuth->stopAuth();
    accept(BIOMETRIC_IGNORE);

}
/*** end of control's slot ***/


/*** public member ***/

void MainWindow::setIcon(const QString &iconName)
{
    QIcon::setThemeName("ukui-icon-theme");
    if(!QIcon::hasThemeIcon("ukui-polkit")) {
        QDir iconsDir("/usr/share/icons");
        auto themesList = iconsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        qDebug() << themesList;
        for(auto theme : themesList) {
            QIcon::setThemeName(theme);
            if(QIcon::hasThemeIcon("ukui-polkit")) {
                qDebug() << theme << "has ukui-polkit icon";
                break;
            }
        }
    }
    QPixmap icon = QIcon::fromTheme("ukui-polkit").pixmap(64, 64);
    QPixmap actionIcon = QIcon::fromTheme(iconName).pixmap(32, 32);
    QPainter painter;

    painter.begin(&icon);
    QRect rect(32,
               32,
               32, 32);
    painter.drawPixmap(rect, actionIcon);
    painter.end();

    setWindowIcon(icon);
    //ui->lblIcon->setPixmap(icon);
}

void MainWindow::setHeader(const QString &text)
{
    ui->lblHeader->setText(text);
    ui->lblHeader->adjustSize();
    ui->lblHeader->height();
    ui->lblContent->setText(tr("An application is attempting to perform an action that requires privileges."
                            " Authentication is required to perform this action."));

}

void MainWindow::setUsers(const QStringList &usersList)
{
    if(usersList.isEmpty())
        return;

    if(usersList.size() == 1) {
        UserItem user = users->getUserByName(usersList.at(0));
        if(user.realName != ""){
            userName = user.realName;
        }else{
            userName = usersList.at(0);
        }
    }

    QList<UserItem> accountUsers = users->getUsers();
    for(QString identifyUser:usersList){
        for(UserItem user:accountUsers){
            if(identifyUser == user.name){
                if(user.realName != "")
                    ui->cmbUsers->addItem(user.realName);
                else
                    ui->cmbUsers->addItem(user.name);
            }
        }
    }

    ui->cmbUsers->show();
    ui->cmbUsers->adjustSize();
    ui->cmbUsers->height();
}
/*
void MainWindow::setDetails(const QString &subPid, const QString &callerPid, const QString &actionId,
                            const QString &actionDesc, const QString vendorName,
                            const QString vendorUrl)
{
    ui->lblSubjectPid->setText(subPid);
    ui->lblCallerPid->setText(callerPid);
    ui->lblActionId->setText(actionId);
    ui->lblActionDesc->setText(actionDesc);
    QString vendor = QString("<a href=\"%1\">%2").arg(vendorUrl).arg(vendorName);
    ui->lblVendor->setText(vendor);
}
*/
void MainWindow::setPrompt(const QString &text, bool echo)
{
    QString prompt = text;

    if(text == "Password: "){
        prompt = tr("Password: ");
        if(useDoubleAuth && widgetBioAuth && widgetBioAuth->isAuthenticating()){
            uid_t m_uid = getUid(userName);
            if(!m_failMap.contains(m_uid) || m_failMap[m_uid] < maxFailedTimes)
                setMessage(tr("Please enter your password or enroll your fingerprint "));
        }
    }
    if(!m_timer){
            m_timer = new QTimer(this);
            m_timer->setInterval(200);
            connect(m_timer, &QTimer::timeout, this, &MainWindow::unlock_countdown);
        }
        //qDebug() << "6666";
        m_timer->start();

    ui->lePassword->clear();
    ui->lePassword->setPlaceholderText(prompt);
    //ui->lblMessage->setText(prompt);
    ui->lePassword->setEchoMode(echo ? QLineEdit::Normal : QLineEdit::Password);
    switchWidget(PASSWORD);
}

/*
    转换pam英文提示，目前转换的就3个翻译
    Authenticated failed, account locked!
    Authenticated failed, 1 login attemps left
    Account locked, 4 minutes left
*/
QString MainWindow::check_is_pam_message(QString text)
{
    setlocale(LC_ALL,"");
    bindtextdomain("Linux-PAM","/usr/share/locale");
    bind_textdomain_codeset("Linux-PAM","UTF-8");
    textdomain("Linux-PAM");
    char*  str;
    QString strTrans = "";
    QByteArray ba = text.toLatin1(); // must
    str=ba.data();

    int a,b;
    if(sscanf(str,"Authenticated failed, %d login attemps left",&a))
          sprintf(str,_("Authenticated failed, %d login attemps left"),a);
    else if(text.contains("days",Qt::CaseSensitive) && sscanf(str,"Account locked, %d days left",&a)){
        strTrans = tr("Account locked,") + QString("%1 ").arg(a) + tr("days left");
        return strTrans;
    }
    else if(text.contains("hours",Qt::CaseSensitive) && sscanf(str,"Account locked, %d hours left",&a)){
        strTrans = tr("Account locked,") + QString("%1 ").arg(a) + tr("hours left");
        return strTrans;
    }
    else if(text.contains("minutes",Qt::CaseSensitive) && sscanf(str,"Account locked, %d minutes left",&a)){
        strTrans = tr("Account locked,") + QString("%1 ").arg(a) + tr("minutes left");
        return strTrans;
    }
    else if(text.contains("seconds",Qt::CaseSensitive) && sscanf(str,"Account locked, %d seconds left",&a)){
        strTrans = tr("Account locked,") + QString("%1 ").arg(a) + tr("seconds left");
        return strTrans;
    }
    else{
        str = _(str);
    }
          
    qDebug()<<"str = "<<str;
    return QString(str);

}

void MainWindow::setMessage(const QString &text,situation situat)
{
    // QString message = this->check_is_pam_message(text);
    if(situat == ERROR){
        ui->lblMessage->setStyleSheet("color: #F3222D;");
    }else if(situat == TRUE){
        ui->lblMessage->setStyleSheet("");
    }
    qDebug()<<"receive：text = "<<text;
    if (text.indexOf("account locked") != -1 || text.indexOf("账户已锁定") != -1 
        || text.indexOf("Account locked") != -1 || text.indexOf("永久锁定") != -1)
    {
        if(!m_timer){
            m_timer = new QTimer(this);
            m_timer->setInterval(400);
            connect(m_timer, &QTimer::timeout, this, &MainWindow::unlock_countdown);
        }
        m_timer->start();
    }else if (text.indexOf("No password received, please input password") != -1)
    {
        ui->lblMessage->setText(tr("Password cannot be empty"));
        ui->lblMessage->setToolTip(tr("Password cannot be empty"));
    }else{
        ui->lblMessage->setText(text);
        ui->lblMessage->setToolTip(text);
    }

    // ui->lblMessage->setText(text);
}

void MainWindow::setAuthResult(bool result, const QString &text)
{
    QString message = text;

    if(!result && text.isEmpty())
        message = tr("Authentication failed, please try again.");

    if(authMode == PASSWORD)
        setMessage(message,ERROR);
    else if(authMode == BIOMETRIC)
        setMessage(message,ERROR);

    switchWidget(PASSWORD);
}

void MainWindow::clearEdit()
{
    ui->lePassword->setText("");
}

void MainWindow::switchAuthMode(Mode mode)
{
    if(useDoubleAuth && isbioSuccess){
        accept(BIOMETRIC_SUCCESS);
        return ;
    }
    int uid = getUid(userName);
    int count = bioDevices.GetUserDevCount(getUid(userName));
    enableBioAuth  = count > 0;
    if(!enable_biometric_authentication()){
    	enableBioAuth = false;
    }
    if(useDoubleAuth && m_failMap.contains(uid) && m_failMap[uid] >= maxFailedTimes){
        enableBioAuth = false;
    }
//    if(mode == BIOMETRIC && enableBioAuth == false)
//        useDoubleAuth = false;

    switch(mode){
    case PASSWORD:
    {
        qDebug() << "switch to password";

        authMode = mode;

        if(!enableBioAuth || !receiveBioPAM || useDoubleAuth) {
            ui->btnBioAuth->hide();
        }else{
            ui->btnBioAuth->show();
        }
	
	if(isHiddenSwitchButton)
	    ui->btnBioAuth->hide();

        if(enableBioAuth && useDoubleAuth){
            DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
            if(device){
                widgetBioAuth->startAuth(getUid(userName), *device);
            }
        }
    }
        break;
    case BIOMETRIC:
    {
        qDebug() << "switch to biometric";

        receiveBioPAM = true;

        if(authMode == PASSWORD) {
            emit accept(BIOMETRIC_IGNORE);
            return;
        }else if(!enableBioAuth){
            qDebug() << "It doesn't meet the condition for enabling biometric authentication, switch to password.";
            emit accept(BIOMETRIC_IGNORE);
            return;
        } else if(authMode == BIOMETRIC) {
            DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
            if(!device)
                device = bioDevices.getFirstDevice();
            if(device) {
                if(useDoubleAuth){
                    emit accept(BIOMETRIC_IGNORE);
                    widgetBioAuth->startAuth(getUid(userName), *device);
                    return;
                }else{
                    widgetBioAuth->startAuth(getUid(userName), *device);
                    widgetBioAuth->setMoreDevices(count > 1);
                }
            }
            authMode = BIOMETRIC;
        } else if(authMode == UNDEFINED){
            authMode = BIOMETRIC;

            if(enableBioAuth) {
                qDebug() << "enable biometric authenticaion";
                DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
                if(device) {
                    if(useDoubleAuth){
                        emit accept(BIOMETRIC_IGNORE);
                        widgetBioAuth->startAuth(getUid(userName), *device);
                        return;
                    }else{
                        widgetBioAuth->startAuth(getUid(userName), *device);
                        widgetBioAuth->setMoreDevices(count > 1);
                    }
                } else {
                    qDebug() << "No default device, switch to password";
                    emit accept(BIOMETRIC_IGNORE);
                    return;
                }

            } else {

                /* pass biometric's pam module if there are not available devices */
                qDebug() << "It doesn't meet the condition for enabling biometric authentication, switch to password.";
                emit accept(BIOMETRIC_IGNORE);
                return;
            }
        }
    }
        break;
    default:
        break;
    }
    switchWidget(mode);
}

/*** end of public memeber ***/


/*** private member ***/

uid_t MainWindow::getUid(const QString &userName)
{
    struct passwd *pwd = getpwnam(userName.toStdString().c_str());
    if(pwd == NULL) {
        qWarning() << "getpwnam error: " << strerror(errno);
        return -1;
    }
    return pwd->pw_uid;
}

void MainWindow::setDoubleAuth(bool val){
     useDoubleAuth = val;
}

void MainWindow::stopDoubleAuth()
{
    if(useDoubleAuth && widgetBioAuth)
        widgetBioAuth->stopAuth();
}

void MainWindow::switchWidget(Mode mode)
{
    ui->widgetPasswdAuth->hide();
    ui->btnAuth->hide();
    widgetBioAuth->hide();
    widgetBioDevices->hide();

    switch(mode){
    case PASSWORD:
        setMinimumWidth(420);
        setMaximumWidth(420);
        setMinimumHeight(211+ui->lblHeader->height()+ui->lblContent->height());
        setMaximumHeight(211+ui->lblHeader->height()+ui->lblContent->height());
        ui->btnBioAuth->setStyleSheet("QPushButton{font-size:14px;}QPushButton:hover{border:none;color:#3E6CE5;}QPushButton:pressed{border:none;}");

        ui->btnBioAuth->setFlat(true);
        ui->widgetPasswdAuth->show();
        ui->lePassword->setFocus();
        ui->lePassword->setAttribute(Qt::WA_InputMethodEnabled, false);
        ui->btnAuth->show();
        ui->btnCancel->show();
        //ui->btnBioAuth->show();
        ui->lblContent->show();
        ui->returnButton->hide();
        //ui->cmbDevices->hide();
        break;
    case BIOMETRIC:
        setMinimumWidth(420);
        setMaximumWidth(420);
        setMinimumHeight(342+ui->cmbUsers->height()+ui->lblHeader->height());
        setMaximumHeight(342+ui->cmbUsers->height()+ui->lblHeader->height());

        widgetBioAuth->show();
        ui->btnCancel->hide();
        ui->lblContent->hide();
        ui->btnBioAuth->hide();
        ui->returnButton->show();
        ui->returnButton->setFlat(true);
        ui->returnButton->setStyleSheet("QPushButton{font-size:14px;}QPushButton:hover{border:none;color:#3E6CE5;}QPushButton:pressed{border:none;}");

        //ui->btnBioAuth->hide();
        //ui->cmbDevices->show();
        break;
    case DEVICES:
        widgetBioAuth->show();

        break;
    default:
        break;
    }
    adjustSize();
}

void MainWindow::unlock_countdown()
{
    int failed_count = 0;
    int time_left = 0;
    int deny = 0;
    int fail_time =0;
    int unlock_time = 0;
    pam_tally_unlock_time_left(&failed_count, &time_left, &deny,&fail_time,&unlock_time);

    // qDebug() << "failed_count:" << failed_count << "time_left:" <<time_left <<"deny:"<<deny<<"fail_time:"<< fail_time<<"unlock_time:" << unlock_time;
    if(time_left/60 > 0)//请多少分钟后重试
    {
        char ch[100]={0};
        int nMinute = time_left/60 + 1;
        setMessage(tr("Please try again in %1 minutes.").arg(nMinute),TRUE);
        setMessage(tr("Please try again in %1 minutes.").arg(nMinute),TRUE);
        ui->lePassword->setText("");
        ui->lePassword->setDisabled(true);
        isLockingFlg = true;
        return ;
    }
    else if(time_left > 0)//请多少秒后重试
    {
        char ch[100]={0};
        setMessage(tr("Please try again in %1 seconds.").arg(time_left%60),TRUE);
        setMessage(tr("Please try again in %1 seconds.").arg(time_left%60),TRUE);
        ui->lePassword->setText("");
        ui->lePassword->setDisabled(true);
        isLockingFlg = true;
        return ;
    }
    else if (failed_count == 0xFFFF)//账号被永久锁定
    {
        ui->lblMessage->setText(tr("Account locked permanently."));
        ui->lblMessage->setToolTip(tr("Account locked permanently."));
        ui->lePassword->setText("");
        ui->lePassword->setDisabled(true);
        isLockingFlg = true;
        return ;
    }
    else
    {
        if(ui->lePassword){
            ui->lePassword->setDisabled(false);
            ui->lePassword->setFocus();
        }

        if (isLockingFlg)
        {
            ui->lblMessage->setText("");
        }
            
        m_timer->stop();
    }
    return ;
}

void MainWindow::root_unlock_countdown()
{
    int failed_count = 0;
    int time_left = 0;
    int deny = 0;
    pam_tally_root_unlock_time_left(&failed_count, &time_left, &deny);

    // qDebug() << "failed_count:" << failed_count << "time_left:" <<time_left <<"deny:"<<deny<<"fail_time:"<< fail_time<<"unlock_time:" << unlock_time;
    if(time_left/60 > 0)//请多少分钟后重试
    {
        char ch[100]={0};
        int nMinute = time_left/60 + 1;
        setMessage(tr("Please try again in %1 minutes.").arg(nMinute),TRUE);
        setMessage(tr("Please try again in %1 minutes.").arg(nMinute),TRUE);
        ui->lePassword->setText("");
        ui->lePassword->setDisabled(true);
        isLockingFlg = true;
        return ;
    }
    else if(time_left > 0)//请多少秒后重试
    {
        char ch[100]={0};
        setMessage(tr("Please try again in %1 seconds.").arg(time_left%60),TRUE);
        setMessage(tr("Please try again in %1 seconds.").arg(time_left%60),TRUE);
        ui->lePassword->setText("");
        ui->lePassword->setDisabled(true);
        isLockingFlg = true;
        return ;
    }
    else if (failed_count == 0xFFFF)//账号被永久锁定
    {
        ui->lblMessage->setText(tr("Account locked permanently."));
        ui->lblMessage->setToolTip(tr("Account locked permanently."));
        ui->lePassword->setText("");
        ui->lePassword->setDisabled(true);
        isLockingFlg = true;
        return ;
    }
    else
    {
        if(ui->lePassword){
            ui->lePassword->setDisabled(false);
            ui->lePassword->setFocus();
        }

        if (isLockingFlg)
        {
            ui->lblMessage->setText("");
        }
            
        m_timer->stop();
    }
    return ;
}

/*** end of private member ***/
