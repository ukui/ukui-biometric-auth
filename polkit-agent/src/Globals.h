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
#ifndef _GLOBALS_H
#define _GLOBALS_H

#define _MULTI_THREADED

#define AGENT_NAME "ukui-polkit-agent"
#define POLKIT_LISTENER_ID "/cn/kylin/PolicyKit1/AuthenticationAgent"

#define BIOMETRIC_PAM       "BIOMETRIC_PAM"
#define BIOMETRIC_IGNORE    "BIOMETRIC_IGNORE"
#define BIOMETRIC_SUCESS    "BIOMETRIC_SUCCESS"
#define BIOMETRIC_FAILED    "BIOMETRIC_FAILED"

#define STR(s) #s
#define GET_STR(s) STR(s)

#endif /* _GLOBALS_H */
