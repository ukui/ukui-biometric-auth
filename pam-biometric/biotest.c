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
#include<security/pam_appl.h>
#include<security/pam_misc.h>
#include<stdio.h>
#include<string.h>
#include <termios.h>
#define BUF_SIZE 128
#define DISABLE_ECHO 0
#define ENABLE_ECHO 1
char * getpasswd(int  enable_echo);
int pam_conversation(int num_msg, const struct pam_message **msg,
		struct pam_response **resp, void *appdata_ptr);

struct pam_conv conv = {
	pam_conversation,
	NULL
};

int pam_conversation(int num_msg, const struct pam_message **msg,
		struct pam_response **resp, void *appdata_ptr)
{
	int count = num_msg;
	char *password = 0;
	*resp = (struct pam_response *)malloc(num_msg *
			sizeof(struct pam_response));
	/*指针备份，下面的循环需要指针后移*/
	struct pam_response *tmp_save = (struct pam_response *)(*resp);
	memset(*resp, 0, num_msg * sizeof(struct pam_response));
	while(count-- >= 1){
		switch((*msg)->msg_style){
			case PAM_PROMPT_ECHO_OFF:
				printf("%s",(*msg)->msg);
				password = getpasswd(DISABLE_ECHO);
				(*resp)->resp = password;
				(*resp)->resp_retcode = 0;
				break;
			case PAM_PROMPT_ECHO_ON:
				printf("%s",(*msg)->msg);
				password = getpasswd(ENABLE_ECHO);
				(*resp)->resp = password;
				(*resp)->resp_retcode = 0;
				break;
			case PAM_ERROR_MSG:
				printf("%s",(*msg)->msg);
				break;
			case PAM_TEXT_INFO:
				printf("%s",(*msg)->msg);
				break;
			default:
				printf("Should not reach here!\n");
		}
		if(count != 0){
			msg++;
			(*resp)++;
		}
	}

	/*指针归位 方法1*/
	/*
	unsigned long address_value = (unsigned long)(*resp) -
		(num_msg -1) * sizeof(struct pam_response);
	(*resp) = (struct pam_response *)address_value;
	*/

	/*指针归位 方法2*/
	(*resp) = tmp_save;

	return PAM_SUCCESS;
}



int main(int argc, char *argv[])
{
	pam_handle_t *pamh = NULL;
	int retval;
	char *user = NULL;
	if(argc == 2){
		user = argv[1];
	} else {
		printf("Usage: You must specify a username.\n");
		exit(1);
	}
	retval = pam_start("biotest", user, &conv, &pamh);
	if(retval == PAM_SUCCESS){
		printf("PAM started Success.\n");
	} else {
		printf("PAM started Failed.\n");
	}

	int auth_status = pam_authenticate(pamh,0);
	if(auth_status != PAM_SUCCESS){
		const char * error_msg = pam_strerror(pamh, auth_status);
		printf("Auth Status : %s\n",error_msg);
	} else {
		printf("Authenticate Success!\n");
	}

	if(pam_end(pamh, retval) != PAM_SUCCESS){
		pamh = NULL;
		printf("Failed to terminate PAM.");
		exit(1);
	}
	return (retval == PAM_SUCCESS ? 0:1);
}

char * getpasswd(int enable_echo)
{
	struct termios tp, save;
	char *password = (char *)malloc(BUF_SIZE * sizeof(char));
	/*
	printf("Input Password:");
	fflush(stdout);
	*/


	if (tcgetattr(STDIN_FILENO, &tp) == -1){
		printf("tcgetattr error\n");
		exit(1);
	}
	save = tp;/* So we can restore settings later */
	if(enable_echo)
		;/*Default is echo. Do nothing*/
	else
		tp.c_lflag &= ~ECHO;/* ECHO off, other bits unchanged */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp) == -1){
		printf("tcgetattr error\n");
		exit(1);
	}

	/* Read some input */

	if (fgets(password, BUF_SIZE, stdin) == NULL)
		printf("Got end-of-file/error on fgets()\n");
	else
		;

	/* Restore original terminal settings */

	if (tcsetattr(STDIN_FILENO, TCSANOW, &save) == -1){
		printf("tcgetattr error\n");
		exit(1);
	}

	/*fgets读入的字符串末尾有回车符，在此替换为\0*/
	password[strlen(password)-1] = '\0';
	return password;
}
