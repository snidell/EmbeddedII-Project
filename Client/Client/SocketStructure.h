
#include <string.h>
#include <stdio.h>
#include <Windows.h>
#include <WinSock.h>
#include <time.h>
#include <conio.h>
char inputChar[300] = "";
int  inputInt[300];
char ParamBuffer[300]="" ;
void generateRandom();
void verifyData();
 typedef struct sp_comm {
	WSADATA wsaData;
	SOCKET cmdrecvsock;
	SOCKET cmdstatusock;
	SOCKET datasock;
	struct sockaddr_in server;
} * sp_comm_t; 

 typedef struct sp_flags {
	unsigned int start_system:1;
	unsigned int pause_system:1;
	unsigned int shutdown_system:1;
	unsigned int analysis_started:1;
	unsigned int restart:1;
	unsigned int transmit_data:1;
} * sp_flags_t;

 typedef struct sp_struct{	
	struct sp_comm		comm;
	struct sp_flags		flags;	
} * sp_struct_t;




#define ERR_CODE_NONE	0	/* no error */
#define ERR_CODE_SWI	1	/* software error */

/////// 
#define CMD_LENGTH		5

#define ARG_NONE		1
#define ARG_NUMBER		2

typedef struct {
	char cmd[CMD_LENGTH];
	int arg;	
} cmd_struct_t;
WSADATA wsaData;

int x = 1;

/* Thread to interface with the ProfileClient */
HANDLE hClientThread; 
DWORD dwClientThreadID;
VOID client_iface_thread(LPVOID parameters);



