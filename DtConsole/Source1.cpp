#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "oldaapi.h"
#include <string.h>
#include <stdio.h>
#include <WinSock.h>
#include <time.h>
#include <math.h>
#include <stddef.h>
#include <iostream>
#include <fstream>
#include <string>
#define NUM_OL_BUFFERS 4
#define ERR_CODE_NONE 0
#define ERR_CODE_SWI 1
#define CMD_LENGTH 5
#define ARG_NONE 1
#define ARG_NUMBER 2
#define ELEMENT_COUNT(X) (sizeof(X) / sizeof((X)[0]))
#define STRLEN 80
#define SHOW_ERROR(ecode) MessageBox(HWND_DESKTOP,olDaGetErrorString(ecode,\
str,STRLEN),"Error", MB_ICONEXCLAMATION | MB_OK);
#define CHECKERROR(ecode) if ((board.status = (ecode)) != OLNOERROR)\
{\
SHOW_ERROR(board.status);\
olDaReleaseDASS(board.hdass);\
olDaTerminate(board.hdrvr);\
return ((UINT)NULL);}
char * process(double signal[],double filter[]);
void convolve(const double signal[], size_t signalLength, const double filter[],
size_t filterLength, double result[]);
double maximum(double array[]);
double minimum(double array[]);
double variance(double * array);
double average(double array []);
void print(const char* name, double array[], size_t length);
void LED(int value);
HANDLE hClientThread;
DWORD dwClientThreadID;
VOID client_iface_thread(LPVOID parameters);
VOID client_iface_sending_thread(LPVOID parameters);
HANDLE hSendThread;
#pragma comment(lib, "ws2_32.lib")
typedef struct sp_comm {
WSADATA wsaData;
SOCKET cmdrecvsock;
SOCKET cmdstatusock;
SOCKET datasock;
struct sockaddr_in server;
} * sp_comm_t;
Page 2 of 14
SERVER.cpp 5/12/15, 9:54 PM
typedef struct sp_flags {
unsigned int start_system:1;
unsigned int pause_system:1;
unsigned int shutdown_system:1;
unsigned int analysis_started:1;
unsigned int restart:1;
unsigned int transmit_data:1;
} * sp_flags_t;
typedef struct sp_struct{
struct sp_comm comm;
struct sp_flags flags;
} * sp_struct_t;
typedef struct {
char cmd[CMD_LENGTH];
int arg;
} cmd_struct_t;
WSADATA wsaData;
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
static BOARD board;
BOOL CALLBACK
GetDriver( LPSTR lpszName, LPSTR lpszEntry, LPARAM lParam )
/*
this is a callback function of olDaEnumBoards, it gets the
strings of the Open Layers board and attempts to initialize
the board. If successful, enumeration is halted.
*/
{
LPBOARD lpboard = (LPBOARD)(LPVOID)lParam;
/* fill in board strings */
lstrcpyn(lpboard->name,lpszName,MAX_BOARD_NAME_LENGTH-1);
lstrcpyn(lpboard->entry,lpszEntry,MAX_BOARD_NAME_LENGTH-1);
/* try to open board */
lpboard->status = olDaInitialize(lpszName,&lpboard->hdrvr);
if (lpboard->hdrvr != NULL)
return FALSE; /* false to stop enumerating */
Page 3 of 14
SERVER.cpp 5/12/15, 9:54 PM
else
return TRUE; /* true to continue */
}
sp_comm_t comm ;
#define CHECKERROR( ecode ) \
do \
{ \
ECODE olStatus; \
if( OLSUCCESS != ( olStatus = ( ecode ) ) ) \
{ \
printf("OpenLayers Error %d\n", olStatus ); \
exit(1); \
} \
} \
while(0)
int lastValue=0;
int first = 1;
int currentValue=0;
int counter = 0;
DBL min=0,max=1;
DBL volts;
ULNG value;
DBL voltsSW;
ULNG valueSW;
ULNG samples;
UINT encoding=0,resolution=0;
PDWORD pBuffer32 = NULL;
PWORD pBuffer = NULL;
char WindowTitle[128];
char temp[128];
int i = 0;
int j = 0;
double filter[100];
char stat[128];
int y;
char inputChar[110] = "";
int firstFlag = 1;
char str[100];
int x = 1;
bool readingSignal = false;
double sampleRate = 0;
char buf[100];
FILE * fp2;
bool settings = false;
UINT channel = 0;
DBL gain = 1.0;
HDASS hAD = NULL;
int a = 0;
int chan = 2;
int k=0;
double convResult[2099];
double buff[2000];
Page 4 of 14
SERVER.cpp 5/12/15, 9:54 PM
double bufHundred[100];
bool filtering = false;
int stopFlag = 1;
FILE * raw = fopen("raw.txt", "a");
FILE * result = fopen("result.txt", "a");
DASSINFO ssinfo;
LRESULT WINAPI
WndProc( HWND hWnd, UINT msg, WPARAM hAD, LPARAM lParam )
{
switch( msg )
{
case OLDA_WM_BUFFER_DONE:
HBUF hBuf;
CHECKERROR (olDaGetBuffer((HDASS)hAD, &hBuf));
if( hBuf && settings && readingSignal)
{
olDaGetRange((HDASS)hAD,&max,&min);
olDaGetEncoding((HDASS)hAD,&encoding);
olDaGetResolution((HDASS)hAD,&resolution);
olDmGetValidSamples( hBuf, &samples ) ;
if (resolution > 16)
{
olDmGetBufferPtr( hBuf,(LPVOID*)&pBuffer32);
value = pBuffer32[samples-1];
}
else
{
olDmGetBufferPtr( hBuf,(LPVOID*)&pBuffer);
valueSW = pBuffer[samples-1];
if (valueSW >= 49000)
{
if (firstFlag)
{
sprintf(inputChar,"DATA ACQUSITION STARTED");
send(comm->datasock, inputChar, sizeof(inputChar), 0);
}
filtering = false;
for (k=0,j=0,i=0;i<samples;i=i+2,j++)
{
value = pBuffer[i]; // direction depends of filter
volts = ((float)max-(float)min)/(1L<<resolution)*value +
(float)min;
buff[j]=volts;
fprintf(raw,"%f\n",volts);
}
convolve(buff,2000,filter,100,convResult);
Page 5 of 14
SERVER.cpp 5/12/15, 9:54 PM
for(i=0;i<2099;i++)
{
fprintf(result,"%f\n",convResult[i]);
}
sprintf(stat,process(buff,filter));
sprintf(inputChar,stat);
send(comm->datasock, inputChar, sizeof(inputChar), 0);
filtering = true;
LED(1);
firstFlag = 0;
stopFlag = 1;
}
else
{
LED(3);
if (stopFlag)
{
sprintf(inputChar,"DATA ACQUSITION STOPPED");
send(comm->datasock, inputChar, sizeof(inputChar), 0);
firstFlag = 1;
stopFlag = 0;
}
}
}
olDaPutBuffer((HDASS)hAD, hBuf);
if (encoding != OL_ENC_BINARY)
{
value ^= 1L << (resolution-1);
value &= (1L << resolution) - 1;
valueSW ^= 1L << (resolution-1);
valueSW &= (1L << resolution) - 1;
}
}
break;
case OLDA_WM_QUEUE_DONE:
printf( "\nAcquisition stopped!" );
PostQuitMessage(0);
break;
case OLDA_WM_TRIGGER_ERROR:
printf( "\nTrigger error: acquisition stopped." );
PostQuitMessage(0);
break;
case OLDA_WM_OVERRUN_ERROR:
printf( "\nInput overrun error: acquisition stopped." );
PostQuitMessage(0);
break;
default:
return DefWindowProc( hWnd, msg, hAD, lParam );
Page 6 of 14
SERVER.cpp 5/12/15, 9:54 PM
}
return 0;
}
BOOL CALLBACK
EnumBrdProc( LPSTR lpszBrdName, LPSTR lpszDriverName, LPARAM lParam )
{
if( OLSUCCESS != ( olDaInitialize( lpszBrdName, (LPHDEV)lParam ) ) )
{
return TRUE;
}
UINT uiCap = 0;
olDaGetDevCaps ( *((LPHDEV)lParam), OLDC_ADELEMENTS, &uiCap );
if( uiCap < 1 )
{
return TRUE;
}
printf( "%s succesfully initialized.\n", lpszBrdName );
return FALSE;
}
int main()
{
board.hdrvr = NULL;
CHECKERROR (olDaEnumBoards(GetDriver,(LPARAM)(LPBOARD)&board));
CHECKERROR (board.status);
if (board.hdrvr == NULL){
MessageBox(HWND_DESKTOP, " No Open Layer boards!!!", "Error",
MB_ICONEXCLAMATION | MB_OK);
}
CHECKERROR (olDaGetDASS(board.hdrvr,OLSS_DOUT,0,&board.hdass));
CHECKERROR (olDaSetDataFlow(board.hdass,OL_DF_SINGLEVALUE));
CHECKERROR (olDaConfig(board.hdass));
CHECKERROR (olDaGetResolution(board.hdass,&resolution));
value = 0;
CHECKERROR (olDaPutSingleValue(board.hdass,value,channel,gain));
CHECKERROR (olDaReleaseDASS(board.hdass));
CHECKERROR (olDaTerminate(board.hdrvr));
int i = 0;
double max;
double min;
double var;
struct sp_struct profiler;
struct sockaddr_in saddr;
struct hostent *hp;
int res = 0;
char ParamBuffer[110];
char inputChar[100] = "";
for (i=0;i<100;i++)
{
bufHundred[i]=0;
Page 7 of 14
SERVER.cpp 5/12/15, 9:54 PM
}
memset(&profiler, 0, sizeof(profiler));
sp_comm_t comm = &profiler.comm;
if ((res = WSAStartup(0x202,&wsaData)) != 0) {
fprintf(stderr,"WSAStartup failed with error %d\n",res);
WSACleanup();
return(ERR_CODE_SWI);
}
hp = (struct hostent*)malloc(sizeof(struct hostent));
hp->h_name = (char*)malloc(sizeof(char)*17);
hp->h_addr_list = (char**)malloc(sizeof(char*)*2);
hp->h_addr_list[0] = (char*)malloc(sizeof(char)*5);
strcpy(hp->h_name, "lab_example\0");
hp->h_addrtype = 2;
hp->h_length = 4;
hp->h_addr_list[0][0] = (signed char)192;
hp->h_addr_list[0][1] = (signed char)168;
hp->h_addr_list[0][2] = (signed char)1;
hp->h_addr_list[0][3] = (signed char)107;
hp->h_addr_list[0][4] = 0;
memset(&saddr, 0, sizeof(saddr));
saddr.sin_family = hp->h_addrtype;
memcpy(&(saddr.sin_addr), hp->h_addr, hp->h_length);
saddr.sin_port = htons(1024);
if ((comm->datasock = socket(AF_INET,SOCK_DGRAM, 0)) == INVALID_SOCKET) {
fprintf(stderr,"socket(datasock) failed: %d\n",WSAGetLastError());
WSACleanup();
return(ERR_CODE_NONE);
}
if (connect(comm->datasock,(struct sockaddr*)&saddr,sizeof(saddr)) ==
SOCKET_ERROR) {
fprintf(stderr,"connect(datasock) failed: %d\n",WSAGetLastError());
WSACleanup();
return(ERR_CODE_SWI);
}
memset(&saddr, 0, sizeof(struct sockaddr_in));
saddr.sin_family = AF_INET;
saddr.sin_addr.s_addr = INADDR_ANY;
saddr.sin_port = htons(1500);
if ((comm->cmdrecvsock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
fprintf(stderr,"socket(cmdrecvsock) failed: %d\n",WSAGetLastError());
WSACleanup();
return(ERR_CODE_NONE);
}
if (bind(comm->cmdrecvsock,(struct sockaddr*)&saddr,sizeof(saddr) ) ==
SOCKET_ERROR) {
fprintf(stderr,"bind() failed: %d\n",WSAGetLastError());
Page 8 of 14
SERVER.cpp 5/12/15, 9:54 PM
WSACleanup();
return(ERR_CODE_NONE);
}
hClientThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)
client_iface_thread, (LPVOID)&profiler, 0, &dwClientThreadID);
SetThreadPriority(hClientThread, THREAD_PRIORITY_LOWEST);
hSendThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)
client_iface_sending_thread, (LPVOID)&profiler, 0, &dwClientThreadID);
SetThreadPriority(hClientThread, THREAD_PRIORITY_LOWEST);
WNDCLASS wc;
memset( &wc, 0, sizeof(wc) );
wc.lpfnWndProc = WndProc;
wc.lpszClassName = "DtConsoleClass";
RegisterClass( &wc );
HWND hWnd = CreateWindow( wc.lpszClassName,
NULL,
NULL,
0, 0, 0, 0,
NULL,
NULL,
NULL,
NULL );
if( !hWnd )
exit( 1 );
while(!settings);
while(!readingSignal);
HDEV hDev = NULL;
CHECKERROR( olDaEnumBoards( EnumBrdProc, (LPARAM)&hDev ) );
CHECKERROR( olDaGetDASS( hDev, OLSS_AD, 0, &hAD ) );
CHECKERROR( olDaSetWndHandle( hAD, hWnd, 0 ) );
CHECKERROR( olDaSetDataFlow( hAD, OL_DF_CONTINUOUS ) );
CHECKERROR( olDaSetChannelListSize( hAD, chan));
CHECKERROR( olDaSetChannelListEntry( hAD, 0, 0));
CHECKERROR( olDaSetChannelListEntry( hAD, 1, 1 ) );
CHECKERROR( olDaSetGainListEntry( hAD, 0, 1 ) );
CHECKERROR( olDaSetGainListEntry( hAD,1, 1 ) );
CHECKERROR( olDaSetTrigger( hAD, OL_TRG_SOFT ) );
CHECKERROR( olDaSetClockSource( hAD, OL_CLK_INTERNAL ) );
CHECKERROR( olDaSetClockFrequency( hAD, sampleRate) );
CHECKERROR( olDaSetWrapMode( hAD, OL_WRP_NONE ) );
CHECKERROR( olDaConfig( hAD ) );
HBUF hBufs[NUM_OL_BUFFERS];
for( int i=0; i < NUM_OL_BUFFERS; i++ )
{
if( OLSUCCESS != olDmAllocBuffer( GHND, sampleRate*chan, &hBufs[i] ) )
{
for ( i--; i>=0; i-- )
Page 9 of 14
SERVER.cpp 5/12/15, 9:54 PM
{
olDmFreeBuffer( hBufs[i] );
}
exit( 1 );
}
olDaPutBuffer( hAD,hBufs[i] );
}
if( OLSUCCESS != ( olDaStart( hAD ) ) )
{
printf( "A/D Operation Start Failed...hit any key to terminate.\n" );
}
MSG msg;
SetMessageQueue( 50 );
while( GetMessage( &msg,
hWnd,
0,
0 ) )
{
TranslateMessage( &msg );
DispatchMessage( &msg );
if( _kbhit() )
{
_getch();
PostQuitMessage(0);
}
}
while(1);
}
VOID client_iface_thread(LPVOID parameters)
{
sp_struct_t profiler = (sp_struct_t)parameters;
sp_comm_t comm = &profiler->comm;
INT retval;
struct sockaddr_in saddr;
int saddr_len;
char ParamBuffer[110] = {0};
int y = 1;
while(ParamBuffer[0] != '!')
{
memset(ParamBuffer, 0, sizeof(ParamBuffer));
saddr_len =sizeof(saddr);
retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0,
(struct sockaddr *)&saddr, &saddr_len);
if(strcmp(ParamBuffer,"START") == 0)
{
sprintf(inputChar,"STARTED. WAITING FOR SWITCH");
send(comm->datasock, inputChar, sizeof(inputChar), 0);
readingSignal = true;
}
if(strcmp(ParamBuffer, "STOP") == 0)
{
LED(0);
sprintf(inputChar,"STOPPED");
Page 10 of 14
SERVER.cpp 5/12/15, 9:54 PM
send(comm->datasock, inputChar, sizeof(inputChar), 0);
readingSignal = false;
sprintf(inputChar,"send");
send(comm->datasock, inputChar, sizeof(inputChar), 0);
FILE* fp = fopen("result.txt","r");
while (fgets(buf,100, fp)!=NULL)
{
sprintf(inputChar,"%s",buf);
send(comm->datasock, inputChar, sizeof(inputChar), 0);
}
fclose(fp);
sprintf(inputChar,"stop");
send(comm->datasock, inputChar, sizeof(inputChar), 0);
}
if(strcmp(ParamBuffer, "s") == 0)
{
retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer)
, 0, (struct sockaddr *)&saddr, &saddr_len);
sampleRate = strtod(ParamBuffer,NULL);
printf("SAMPLING RATE: %f\n", sampleRate);
sprintf(inputChar,"SAMPLE RATE RECEIVED");
send(comm->datasock, inputChar, sizeof(inputChar), 0);
}
if(strcmp(ParamBuffer, "f") == 0)
{
for(i=0;i<100;i++)
{
retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof
(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
filter[i] = atof(ParamBuffer);
}
sprintf(inputChar,"FILTER RECEIVED");
send(comm->datasock, inputChar, sizeof(inputChar), 0);
//print("FILTER:\n", filter, ELEMENT_COUNT(filter));
}
if(strcmp(ParamBuffer, "r") == 0)
{
retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer)
, 0, (struct sockaddr *)&saddr, &saddr_len);
printf("RESULT FILE NAME: %s\n", ParamBuffer);
sprintf(inputChar,"FILE NAME RECEIVED");
send(comm->datasock, inputChar, sizeof(inputChar), 0);
sprintf(inputChar,"WAITING FOR START");
send(comm->datasock, inputChar, sizeof(inputChar), 0);
settings = true;
}
}x
= 0;
}
VOID client_iface_sending_thread(LPVOID parameters) //LPVOID parameters)
{
sp_struct_t profiler = (sp_struct_t)parameters;
comm = &profiler->comm;
INT retval;
Page 11 of 14
SERVER.cpp 5/12/15, 9:54 PM
struct sockaddr_in saddr;
int saddr_len;
char inputChar[110] = {0};
}
char * process(double signal[], double filter[])
{
double signalTemp[2000];
double filterTemp[100];
int i;
for(i=0;i<2000;i++)
{
signalTemp[i] = signal[i];
}
for(i=0;i<100;i++)
{
filterTemp[i] = filter[i];
}
double result[2000 + 100 - 1];
convolve(signalTemp, 2000, filterTemp, 100, result);
double max = maximum(result);
double min = minimum(result);
double var = variance(result);
double avg = average(result);
char stat[128];
sprintf(stat,"MAX:%f MIN:%f AVG:%f VAR:%f",max,min,avg,var);
return(stat);
}
void convolve(const double signal[], size_t signalLength, const double filter[],
size_t filterLength, double result[])
{
size_t n;
for (n = 0; n < signalLength + filterLength - 1; n++)
{
size_t kmin, kmax, k;
result[n] = 0;
kmin = (n >= filterLength - 1) ? n - (filterLength - 1) : 0;
kmax = (n < signalLength - 1) ? n : signalLength - 1;
for (k = kmin; k <= kmax; k++)
result[n] += signal[k] * filter[n - k];
}
}
void convoluteRealTime(const double signal[], size_t signalLength, const double
filter[], size_t filterLength,double finalResult[])
{
double result[2099];
size_t n;
for (n = 0; n < signalLength + filterLength - 1; n++)
{
size_t kmin, kmax, k;
result[n] = 0;
kmin = (n >= filterLength - 1) ? n - (filterLength - 1) : 0;
kmax = (n < signalLength - 1) ? n : signalLength - 1;
Page 12 of 14
SERVER.cpp 5/12/15, 9:54 PM
for (k = kmin; k <= kmax; k++)
result[n] += signal[k] * filter[n - k];
}
int j = 0;
for (int i=filterLength-1;i<=signalLength-filterLength;i++)
{
finalResult[j]=result[i];
j++;
}
}
void nextBuf(const double prev[], const double next[], size_t length, size_t
filterLength, double newBuf[])
{
for(int i=0;i<=length+filterLength;i++)
{
if((i>=0)&&(i<filterLength))
newBuf[i]=prev[length-filterLength+i];
else
{
newBuf[i]=next[i-filterLength];
}
}
}
double maximum(double array[])
{
int length = 1099;
double max = array[200];
int i;
for(i=200; i<length-200; i++)
{
if(array[i] > max)
{
max = array[i];
}
}
return max;
}
double minimum(double array[])
{
int length = 1099;
double min = array[200];
int i;
for(i=200; i<length-200; i++)
{
if(array[i] < min)
{
min = array[i];
}
}
return min;
}
double average(double array [])
Page 13 of 14
SERVER.cpp 5/12/15, 9:54 PM
{
int length = 2099;
double sum=0,average;
int i;
for(i=200; i<length-200; i++)
{
sum += array[i];
}
average = sum/length;
return average;
}
double variance(double * array)
{
int length = 1099;
double sum1=0,average,sum2=0,variance;
int i;
for(i=200; i<length-200; i++)
{
sum1 += array[i];
}
average = sum1/length;
for(i=200; i<length-200; i++)
{
sum2 = sum2 + pow((array[i] - average), 2);
}
variance = sum2/length;
return variance;
}
void print(const char* name, double array[], size_t length)
{
size_t i;
printf("%s",name);
for (i = 0; i < length; i++)
printf("%f ",array[i]);
printf("\n");
}
void LED(int value)
{
board.hdrvr = NULL;
CHECKERROR (olDaEnumBoards(GetDriver,(LPARAM)(LPBOARD)&board));
CHECKERROR (board.status);
if (board.hdrvr == NULL){
MessageBox(HWND_DESKTOP, " No Open Layer boards!!!", "Error",
MB_ICONEXCLAMATION | MB_OK);
}
CHECKERROR (olDaGetDASS(board.hdrvr,OLSS_DOUT,0,&board.hdass));
CHECKERROR (olDaSetDataFlow(board.hdass,OL_DF_SINGLEVALUE));
CHECKERROR (olDaConfig(board.hdass));
CHECKERROR (olDaGetResolution(board.hdass,&resolution));
CHECKERROR (olDaPutSingleValue(board.hdass,value,channel,gain));
CHECKERROR (olDaReleaseDASS(board.hdass));
CHECKERROR (olDaTerminate(board.hdrvr));
Page 14 of 14
SERVER.cpp 5/12/15, 9:54 PM
}