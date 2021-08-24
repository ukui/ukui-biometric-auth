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
#include "bioauthwidget.h"
#include "ui_bioauthwidget.h"
#include <QMovie>
#include "generic.h"
#include <unistd.h>
#include <pwd.h>
#include "giodbus.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

BioAuthWidget::BioAuthWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BioAuthWidget),
    bioAuth(nullptr)
{
    ui->setupUi(this);
    ui->btnRetry->setFlat(true);
    ui->btnRetry->setVisible(false);
    ui->btnRetry->setText(tr("Retry"));
    //ui->btnMore->setVisible(false);
}

BioAuthWidget::~BioAuthWidget()
{
    delete ui;
}

void BioAuthWidget::emitSwithToPassword()
{
    Q_EMIT switchToPassword();
}

void BioAuthWidget::on_btnPasswdAuth_clicked()
{
    stopAuth();
    //Q_EMIT switchToPassword();
    QTimer::singleShot(100,this,SLOT(emitSwithToPassword()));
}

void BioAuthWidget::on_btnMore_clicked()
{
    Q_EMIT selectDevice();
}

void BioAuthWidget::on_btnRetry_clicked()
{
    if(bioAuth && !bioAuth->isAuthenticating()) {
        setMovie();
        bioAuth->startAuth();
    }
}

void BioAuthWidget::onBioAuthNotify(const QString &notifyMsg)
{
    ui->lblBioNotify->setText(notifyMsg);
}

void BioAuthWidget::onBioAuthComplete(uid_t uid, bool ret)
{
    setImage();
    dup_fd = -1;
    ui->btnRetry->setVisible(true);
    Q_EMIT authComplete(uid, ret);
}

void BioAuthWidget::setMovie()
{
    if(device.biotype == BIOTYPE_FACE)
        return ;

    QString typeString = bioTypeToString(device.biotype);
    QString moviePath = QString("%1/images/%2.gif").arg(GET_STR(UKUI_BIOMETRIC)).arg(typeString);
    QMovie *movie = new QMovie(moviePath);
    movie->setScaledSize(QSize(ui->lblBioImage->width(), ui->lblBioImage->height()));

    ui->lblBioImage->setMovie(movie);
    movie->start();

    ui->btnRetry->setVisible(false);

    qDebug() << "set movie " << moviePath;
}

void BioAuthWidget::setImage()
{
    if(device.biotype == BIOTYPE_FACE)
        return ;
    QString typeString = bioTypeToString(device.biotype);
    QString pixmapPath = QString("%1/images/%2.png").arg(GET_STR(UKUI_BIOMETRIC)).arg(typeString);
    QPixmap pixmap(pixmapPath);
    pixmap = pixmap.scaled(ui->lblBioImage->width(), ui->lblBioImage->height(),
                           Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->lblBioImage->setPixmap(pixmap);

    ui->btnRetry->setVisible(true);

    qDebug() << "set pixmap " << typeString;
}

void BioAuthWidget::onFrameWritten(int deviceId)
{
    if(dup_fd == -1){
        dup_fd = get_server_gvariant_stdout(deviceId);
    }

    cv::Mat img;
    lseek(dup_fd, 0, SEEK_SET);
    char base64_bufferData[1024*1024];
    int rc = read(dup_fd, base64_bufferData, 1024*1024);
    printf("rc = %d\n", rc);

    cv::Mat mat2(1, sizeof(base64_bufferData), CV_8U, base64_bufferData);
    img = cv::imdecode(mat2, cv::IMREAD_COLOR);

    QImage srcQImage = QImage((uchar*)(img.data), img.cols, img.rows, QImage::Format_RGB888);
    ui->lblBioImage->setPixmap(QPixmap::fromImage(srcQImage).scaled(ui->lblBioImage->size()));

    ui->btnRetry->setVisible(false);
}

void BioAuthWidget::hidePasswdButton()
{
    //ui->btnPasswdAuth->hide();
}

bool BioAuthWidget::isAuthenticating()
{
    if(bioAuth){
    	return bioAuth->isAuthenticating();
    }
    return false;
}

void BioAuthWidget::stopAuth()
{
    if(bioAuth){
        bioAuth->stopAuth();
    }
}

void BioAuthWidget::startAuth(uid_t uid, const DeviceInfo &device)
{
    this->uid = uid;
    this->device = device;

    //ui->lblBioDevice->setText(tr("Current Device: ") + device.device_shortname);

    if(bioAuth) {
        bioAuth->stopAuth();
        delete bioAuth;
        bioAuth = nullptr;
    }

    setMovie();

    bioAuth = new BioAuth(uid, device, this);
    connect(bioAuth, &BioAuth::notify, this, &BioAuthWidget::onBioAuthNotify);
    connect(bioAuth, &BioAuth::authComplete, this, &BioAuthWidget::onBioAuthComplete);
    connect(bioAuth, &BioAuth::frameWritten,this,&BioAuthWidget::onFrameWritten);

    dup_fd = -1;
    fd = -1;
    bioAuth->startAuth();

}

void BioAuthWidget::setMoreDevices(bool hasMore)
{
    //ui->btnMore->setVisible(hasMore);
}
