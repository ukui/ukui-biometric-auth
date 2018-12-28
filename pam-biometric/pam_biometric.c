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
#define PAM_SM_AUTH
#include "generic.h"
#include <stdio.h>
#include <unistd.h>
#include <security/pam_modules.h>
#include <security/_pam_macros.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <security/pam_ext.h>
#include <signal.h>
#include <errno.h>

/* Declare log function */
extern int enable_debug;
extern char *log_prefix;
extern int logger(char *format, ...);

/* GUI child process alive status */
static int child_alive = 1;
/* Signal handler */
static void signal_handler(int signo)
{
    if (signo == SIGUSR1)
    	child_alive = 0; /* GUI child process has terminated */
    logger("signal_handler is triggered\n");
}

/*
 * Check if the service should use biometric authentication
 */
int service_filter(char *service)
{
    if (strcmp(service, "lightdm") == 0) {
    	int ret = system("/bin/chmod -f a+wr /tmp/bio.log");
    	(void)ret; /* Suppress gcc ignoring return value warning */
    	return 1;
    }
    if (strcmp(service, "ukui-screensaver-qt") == 0)
    	return 1;
    if (strcmp(service, "sudo") == 0)
    	return 1;
    if (strcmp(service, "su") == 0)
    	return 1;
//    if (strcmp(service, "mate-screensaver") == 0)
//    	return 1;
    if (strcmp(service, "polkit-1") == 0)
    	return 1;
#ifdef ENABLE_BIOTEST
    if (strcmp(service, "biotest") == 0)
    	return 1;
#endif
    return 0;
}

/*
 * Invoke the PAM conversation function
 */
int call_conversation(pam_handle_t *pamh, int msg_style, char *msg, char *resp)
{
    /* PAM data structures used by conversation */
    const struct pam_message *message[1] = {0};
    struct pam_message *message_tmp = 0;
    struct pam_response *response = 0;
    const struct pam_conv *conv_struct = 0;
    int status = -1;

    status = pam_get_item(pamh, PAM_CONV, (const void **)&conv_struct);
    if (status != PAM_SUCCESS)
    	return PAM_SYSTEM_ERR;

    message_tmp = (struct pam_message *)malloc(sizeof(struct pam_message));

    message_tmp->msg_style = msg_style;
    message_tmp->msg = msg;
    message[0] = message_tmp;
    logger("Call conv callback function\n");
    status = conv_struct->conv(1, message, &response, conv_struct->appdata_ptr);
    logger("Finish conv callback function\n");

        if (resp)
            strcpy(resp, response->resp);
    /* Use typecast to suppress gcc warnings */
    free((void *)message[0]);
    if (response)
    	free(response->resp);
    free(response);

    return status;
}

/* GUI child process */
void child(char *service, char *username, char *xdisp)
{
    char *gui = "/usr/bin/bioauth";
    logger("Child process will be replaced.\n");
    int fd = open("/dev/null", O_WRONLY);
    dup2(fd, 2);

    execl(gui, "bioauth",
          "--service", service,
          "--user", username,
          "--display", xdisp,
          enable_debug ? "--debug" : "",
          (char *)0);
    /*
     * execl almost always succeed as long as the GUI executable file exists.
     * Though invoking GUI under console will exit with error, the GUI child
     * process won't reach here. Therefore, the following code won't be
     * executed in general.
     */
    logger("Fatal error: execl(gui) failed in child process. "
    	"This is an extremely rare condition. Please ensure that the "
        "biometric-authentication executable file exists.\n");
    logger("Use password as a fallback\n");
    logger("Child _exit with BIO_IGNORE\n");
    /* Child process exits */
    _exit(BIO_IGNORE);
}

/* PAM parent process */
int parent(int pid, pam_handle_t *pamh, int need_call_conv)
{
    logger("Parent process continue running.\n");
    int child_status = -1;
    /*
     * 1. If calling conversation function is not needed, wait the child
     * until it exits.
     * 2. Otherwise, send a text info to application at first and then
     * call the conversation function to request the password. The returned
     * password is unused.
     * Note: During requesting the password, screensaver won't display the
     * prompting message, while greeter will display it into the password
     * entry. If there is a "\n" at the end of the message, it will be
     * displayed on the Label above the password entry.
     */
    if (need_call_conv) {
    	char *lang = getenv("LANG");
    	char *msg1, *msg2;
    	if (lang && !strncmp(lang, "zh_CN", 5))
    		msg1 = "请进行生物识别或点击“切换到密码登录”\n";
    	else
            msg1 = "Use biometric authentication or click "
    					"\"Switch to password\"\n";
    	#ifdef EMPTY_PAM_PWD_PROMPT
    		msg2 = "";
    	#else
    		msg2 = "pam_biometric.so needs a fake ENTER:";
    	#endif

    	if (signal(SIGUSR1, signal_handler) == SIG_ERR)
            logger("Fatal Error. Can't catch SIGUSR1\n");
    reinvoke:
                call_conversation(pamh, PAM_TEXT_INFO, msg1, NULL);
                call_conversation(pamh, PAM_PROMPT_ECHO_OFF, msg2, NULL);
    	/* GUI child process is still alive. This enter is typed by user. */
    	if (child_alive == 1)
    		goto reinvoke;
    	signal(SIGUSR1, SIG_DFL);
    	waitpid(pid, &child_status, 0);
    } else {
        logger("Waiting for the GUI child process to exit...\n");
    	waitpid(pid, &child_status, 0);
        logger("GUI child process has exited.\n");
    }

    /*
     * Determine the return value of PAM according to the return value of
     * child process.
     */
    int bio_result = BIO_ERROR; /* biometric result code */
    if (WIFEXITED(child_status))
    	bio_result = WEXITSTATUS(child_status);
    else /* This may be because the GUI child process is invoked under console. */
        logger("The GUI-Child process terminate abnormally.\n");

    if (bio_result == BIO_SUCCESS) {
        logger("pam_biometric.so return PAM_SUCCESS\n");
    	return PAM_SUCCESS;
    } else if (bio_result == BIO_IGNORE) {
    	/* Override msg1 to empty the label. We are ready to enter the password module. */
                call_conversation(pamh, PAM_TEXT_INFO, "", NULL);
        logger("pam_biometric.so return PAM_IGNORE\n");
    	return PAM_IGNORE;
    } else {
        logger("pam_biometric.so return PAM_SYSTEM_ERR\n");
    	return PAM_SYSTEM_ERR;
    }
}

/* Set environment variables related to displaying */
void check_and_set_env(pam_handle_t *pamh, char **xdisp, char **xauth)
{
    *xdisp=getenv("DISPLAY");
    *xauth=getenv("XAUTHORITY");

    if (*xdisp == 0){
    	pam_get_item(pamh, PAM_XDISPLAY, (const void **)xdisp);
    	if (*xdisp)
    		setenv("DISPLAY",*xdisp,-1);
    }
    if (*xauth == 0)
    	setenv("XAUTHORITY", "/var/run/lightdm/root/:0", -1);

    *xdisp=getenv("DISPLAY");
    *xauth=getenv("XAUTHORITY");
    if (*xdisp == 0)
        logger("Warning: DISPLAY env is still empty, "
    		"this is not an error if you are using terminal\n");
    if (*xauth == 0)
        logger("Warning: XAUTHORITY env is still empty, "
    		"this is not an error if you are using terminal\n");

}

/* Biometric processing function for generic purpose */
int biometric_auth_independent(pam_handle_t *pamh , char *service, int need_call_conv)
{
    /* Get the username */
    char *username = 0;
    pam_get_item(pamh, PAM_USER, (const void **)&username);

    /* Check and set environment variables */
    char *xdisp, *xauth;
    check_and_set_env(pamh, &xdisp, &xauth);

    /* Detach child process */
    unsigned int pid;
    pid = fork();
    if (pid == 0 ) {
    	child(service, username, xdisp);
        logger("Should never reach here.\n");
    	return PAM_SYSTEM_ERR;
    } else if (pid > 0) {
    	return parent(pid, pamh, need_call_conv);
    } else {
        logger("Fork Error!\n");
    	return PAM_SYSTEM_ERR;
    }
}

/* Biometric processing function fot polkit-1 */
int biometric_auth_polkit()
{
    logger("Current service is polkit-1\n");
    const char *fifo_name = "/tmp/bio.fifo";
    if(access(fifo_name, F_OK) == -1) {
    	int res = mkfifo(fifo_name, 0777);
    	if(res != 0) {
            logger("Can't create FIFO file\n");
    		return PAM_SYSTEM_ERR;
    	}
    }
    int fifo_rd = open(fifo_name, O_RDONLY);
    if (fifo_rd == -1)
    	return PAM_SYSTEM_ERR;
    logger("Before reading FIFO\n");
    char buffer[8] = {0};
    if(read(fifo_rd, buffer, 8) == -1)
    	return PAM_SYSTEM_ERR;
    logger("After reading FIFO\n");
    int result_code;
    sscanf(buffer, "%d", &result_code);
    remove(fifo_name);
    if (result_code == BIO_SUCCESS) {
        logger("pam_biometric.so return PAM_SUCCESS\n");
    	return PAM_SUCCESS;
    } else if (result_code == BIO_IGNORE) {
        logger("pam_biometric.so return PAM_IGNORE\n");
    	return PAM_IGNORE;
    } else {
        logger("pam_biometric.so return PAM_SYSTEM_ERR\n");
    	return PAM_SYSTEM_ERR;
    }
}

int biometric_auth_embeded(pam_handle_t *pamh)
{
    /*
     * By convention, PAM module sends a string "BIOMETRIC_PAM" to
     * lightdm and this message will be forwarded to greeter. After
     * the authentication is completed, greeter will send a string
     * "BIOMETRIC_IGNORE"/"BIOMETRIC_SUCCESS" to lightdm and then it
     * will be forwarded to PAM module. We can get the authentication
     * status by comparing strings.
     */
    char resp[96] = {0};
    call_conversation(pamh, PAM_PROMPT_ECHO_OFF, BIOMETRIC_PAM, resp);

    if (strcmp(resp, BIOMETRIC_IGNORE) == 0)
        return PAM_IGNORE;
    else if (strcmp(resp, BIOMETRIC_SUCCESS) == 0)
        return PAM_SUCCESS;
    else if (strcmp(resp, BIOMETRIC_FAILED) == 0)
        return PAM_AUTH_ERR;
    else
        return PAM_SYSTEM_ERR;
}

void get_greeter_session(char buf[], int len)
{
    FILE *stream;
    char cmd[] = "ps -aux | grep greeter-session | grep -v grep | awk '{print $13}' | awk -F '/' '{print $4}'";
    memset(buf, 0, len);
    stream = popen(cmd, "r");
    if(fgets(buf, len, stream) == NULL)
        logger("get greeter session error: %d\n", errno);
    buf[strlen(buf)-1] = '\0';
    if(strlen(buf) == 0) {
    char cmd1[] = "ps aux | grep ukui-greeter | grep -v grep | wc -l";
    pclose(stream);
    stream = popen(cmd1, "r");
    if(fgets(buf, len, stream) == NULL)
        logger("get greeter session error: %d\n", errno);
    int i = atoi(buf);
    if(i > 0)
        strcpy(buf, "ukui-greeter");
    }
    pclose(stream);
}

int enable_by_polkit()
{
    FILE *file;
    char buf[1024];

    if( (file = fopen(BIO_COM_FILE, "r")) == NULL) {
        logger("open communication file failed: %s\n", strerror(errno));
        return 0;
    }
    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), file);
    fclose(file);
    if(remove(BIO_COM_FILE) < 0)
        logger("remove communication file failed: %s\n", strerror(errno));
    logger("%s\n", buf);
    if(strcmp(buf, "polkit-ukui-authentication-agent-1") == 0)
        return 1;
    return 0;
}

int enable_biometric_authentication()
{
    char conf_file[] = GET_STR(CONFIG_FILE);
    FILE *file;
    char line[1024], is_enable[16];
    int i;


    if((file = fopen(conf_file, "r")) == NULL){
        logger("open configure file failed: %s\n", strerror(errno));
        return 0;
    }
    while(fgets(line, sizeof(line), file)) {
        i = sscanf(line, "EnableAuth=%s\n",  is_enable);
        if(i > 0) {
            logger("EnableAuth=%s\n", is_enable);
            break;
        }
    }
    
    fclose(file);
    if(!strcmp(is_enable, "true"))
        return 1;
    return 0;
}

/*
 * SPI interface
 */
int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc,
        const char **argv)
{
    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "debug") == 0) {
            enable_debug = 1;
            log_prefix = "PAM_BIO";
        }
    }

    logger("Invoke libpam_biometric.so module\n");

    char *service = 0;

    if(!enable_biometric_authentication()) {
        logger("disable biometric authentication.\n");
    	return PAM_IGNORE;
    }
    logger("enable biometric authentication.\n");

    pam_get_item(pamh, PAM_SERVICE, (const void **)&service);

    /* Service filter */
    if (!service_filter(service)){
        logger("Service <%s> should not use biometric-authentication\n", service);
    	return PAM_IGNORE;
    }



    /* Different services use different processing function */
    if (strcmp(service, "lightdm") == 0) {
        char buf[128];
        get_greeter_session(buf, sizeof(buf));
        logger("current greeter: %s\n", buf);

        if(strcmp(buf, "ukui-greeter") == 0)
            return biometric_auth_embeded(pamh);
//        else
//            return biometric_auth_independent(pamh, "lightdm", 1);
    }
    else if (strcmp(service, "ukui-screensaver-qt")==0)
        return biometric_auth_embeded(pamh);
    else if (strcmp(service, "polkit-1") == 0){
        if(enable_by_polkit())
            return biometric_auth_embeded(pamh);
        else
            logger("[PAM_BIOMETRIC]: It's not polkit-ukui-authentication-agent-1.\n");
    }
    else if (strcmp(service, "sudo") == 0)
        return biometric_auth_independent(pamh, "sudo", 0);
    else if (strcmp(service, "su") == 0)
        return biometric_auth_independent(pamh, "su", 0);
//    else if (strcmp(service, "mate-screensaver") == 0)
//        return biometric_auth_independent(pamh, "mate-screensaver", 1);
    #ifdef ENABLE_BIOTEST
    else if (strcmp(service, "biotest") == 0)
        return biometric_auth_independent(pamh, "biotest", 1);
    #endif
    else
        logger("Service <%s> slip through the service filter\n", service);
    return PAM_IGNORE;
}
