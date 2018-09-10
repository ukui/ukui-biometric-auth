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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int enable_debug;
char *log_prefix;

void logger(char *format, ...)
{
    if(!enable_debug){
        return;
    }

	va_list args;

	time_t t = time(NULL);
	char timestr[32] = {0};
	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&t)); //产生"YYYYMMDD hh:mm:ss"格式的字符串。

    fprintf(stderr, "[%s] %s - ", log_prefix, timestr);
	va_start(args, format); /* 初始化 args */
	vfprintf(stderr, format, args);
}
