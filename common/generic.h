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
#ifndef GENERIC_H
#define GENERIC_H

#define BIO_ERROR -1
#define BIO_FAILED 0
#define BIO_SUCCESS 1
#define BIO_IGNORE 2

#define BIOMETRIC_PAM       "BIOMETRIC_PAM"
#define BIOMETRIC_IGNORE    "BIOMETRIC_IGNORE"
#define BIOMETRIC_SUCCESS    "BIOMETRIC_SUCCESS"
#define BIOMETRIC_FAILED    "BIOMETRIC_FAILED"

#define BIO_COM_FILE "/tmp/bio_com"

#define STR(s) #s
#define GET_STR(s) STR(s)


#define _MULTI_THREADED

#define AGENT_NAME "polkit-ukui-authentication-agent"
#define POLKIT_LISTENER_ID "/org/ukui/PolicyKit1/AuthenticationAgent"


#ifdef __cplusplus

#include "qlogging.h"

extern bool enableDebug;
extern QString logPrefix;
void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg);

#endif


#endif
