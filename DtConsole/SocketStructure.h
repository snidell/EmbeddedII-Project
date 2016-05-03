#include <string.h>
#include <stdio.h>
#include <WinSock.h>
#include <time.h>
#include <conio.h>


/*--Socket stuff--**/

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

/*--End Socket stuff--**/

typedef struct tag_board {
	HDEV hdrvr; /* device handle */
	HDASS hdass; /* sub system handle */
	ECODE status; /* board error status */
	HBUF hbuf; /* sub system buffer handle */
	PWORD lpbuf; /* buffer pointer */
	char name[MAX_BOARD_NAME_LENGTH]; /* string for board name */
	char entry[MAX_BOARD_NAME_LENGTH]; /* string for board name */
} BOARD;
typedef BOARD* LPBOARD;
BOOL CALLBACK
GetDriver( LPSTR lpszName, LPSTR lpszEntry, LPARAM lParam ){
	LPBOARD lpboard = (LPBOARD)(LPVOID)lParam;
	/* fill in board strings */
	lstrcpyn(lpboard->name,lpszName,MAX_BOARD_NAME_LENGTH-1);
	lstrcpyn(lpboard->entry,lpszEntry,MAX_BOARD_NAME_LENGTH-1);
	/* try to open board */
	lpboard->status = olDaInitialize(lpszName,&lpboard->hdrvr);
	if (lpboard->hdrvr != NULL)
		return FALSE; /* false to stop enumerating */		
	else
		return TRUE; /* true to continue */
}

