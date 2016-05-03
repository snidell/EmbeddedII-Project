#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "oldaapi.h"
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <stdio.h>
#include <WinSock.h>
#include <time.h>
#include <math.h>
#include <stddef.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>


#include <string>
#include <iostream>

//Global Values i am adding-------------------------------------------------------------
#define NUM_OL_BUFFERS 4
#define ERR_CODE_NONE 0
#define ERR_CODE_SWI 1
#define CMD_LENGTH 5
#define ARG_NONE 1
#define ARG_NUMBER 2
#define STRLEN 80

HWND hWnd;
HBUF hBufs[NUM_OL_BUFFERS];
HDEV hDev;



int lastValue=0;
int first = 1;
int currentValue=0;
int counter = 0;
DBL min=0,max=1;
DBL volts;
ULNG value;
DBL voltsSW;
ULNG switchValue;
ULNG samples;
UINT encoding=0,resolution=0;
PDWORD pBuffer32 = NULL;
PWORD pBuffer = NULL;
char WindowTitle[128];
char temp[128];
int i = 0;
int j = 0;
std::vector<double>filter;

char stat[128];
int y;
char inputChar[300] = "";
char myChar[1024]="";
int readyFlag = 1;
char str[100];
int x = 1;
bool startSignal = false;
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
std::vector<double> rawSignal;
int transientOverAmount;
std::vector<double> transientOverFlow(transientOverAmount,0);
double bufHundred[100];
bool filtering = false;
int processingStopped = 1;



FILE * raw = fopen("raw.txt", "a");
FILE * result = fopen("result.txt", "a");
DASSINFO ssinfo;


//Methods i am adding--------------------------------------------------------------------
void setLED(int level);
int intilializeBoard();
void startDAQ();
std::vector<double> filterSignal(std::vector<double> filter, std::vector<double> signal,std::vector<double> transientOverFlow,int transientOverAmount);
const char* getStats(std::vector<double>filteredSignal);
std::string getMax(std::vector<double>filteredSignal);
std::string getMin(std::vector<double>filteredSignal);
double getAverage(std::vector<double>filteredSignal);
std::string getVariance(std::vector<double>filteredSignal);

//DT Console Structs---------------------------------------------------------------------

#define SHOW_ERROR(ecode) MessageBox(HWND_DESKTOP,olDaGetErrorString(ecode,\
str,STRLEN),"Error", MB_ICONEXCLAMATION | MB_OK);
#define CHECKERROR(ecode) if ((board.status = (ecode)) != OLNOERROR)\
{\
	SHOW_ERROR(board.status);\
	olDaReleaseDASS(board.hdass);\
	olDaTerminate(board.hdrvr);\
	return ((UINT)NULL);}

#define CHECKERROR( ecode )                           \
do                                                    \
{                                                     \
   ECODE olStatus;                                    \
   if( OLSUCCESS != ( olStatus = ( ecode ) ) )        \
   {                                                  \
      printf("OpenLayers Error %d\n", olStatus );     \
	  system("poause");                               \
      exit(1);                                        \
   }                                                  \
}                                                     \
while(0)      

#define NUM_OL_BUFFERS 4



//Network Globals with threading----------------------------------------------------------------
HANDLE hClientThread;
DWORD dwClientThreadID;
VOID client_iface_thread(LPVOID parameters);
VOID client_iface_sending_thread(LPVOID parameters);
HANDLE hSendThread;
#pragma comment(lib, "ws2_32.lib")

//Network Structs with threading----------------------------------------------------------------
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
	else
	return TRUE; /* true to continue */
}


BOOL CALLBACK 
EnumBrdProc( LPSTR lpszBrdName, LPSTR lpszDriverName, LPARAM lParam )
{

   // Make sure we can Init Board
   if( OLSUCCESS != ( olDaInitialize( lpszBrdName, (LPHDEV)lParam ) ) )
   {
      return TRUE;  // try again
   }

   // Make sure Board has an A/D Subsystem 
   UINT uiCap = 0;
   olDaGetDevCaps ( *((LPHDEV)lParam), OLDC_ADELEMENTS, &uiCap );
   if( uiCap < 1 )
   {
      return TRUE;  // try again
   }

   printf( "%s succesfully initialized.\n", lpszBrdName );
   return FALSE;    // all set , board handle in lParam
}

sp_comm_t comm;
LRESULT WINAPI 
WndProc( HWND hWnd, UINT msg, WPARAM hAD, LPARAM lParam )
{
	
	std::cout<<"wndProc called"<<std::endl;
	std::cout<<"MSG: "<<msg<<std::endl;
    switch( msg )
	{
		case OLDA_WM_BUFFER_DONE:
		HBUF hBuf;
		CHECKERROR (olDaGetBuffer((HDASS)hAD, &hBuf));
		if( hBuf && settings && startSignal)
		{
			
			CHECKERROR(olDaGetRange((HDASS)hAD,&max,&min));
			
			CHECKERROR(olDaGetEncoding((HDASS)hAD,&encoding));
			
			CHECKERROR(olDaGetResolution((HDASS)hAD,&resolution));
			
			CHECKERROR(olDmGetValidSamples( hBuf, &samples )) ;
			
			if (resolution > 16)
			{
				CHECKERROR(olDmGetBufferPtr( hBuf,(LPVOID*)&pBuffer32));
				value = pBuffer32[samples-1];
			}
			else
			{
				CHECKERROR(olDmGetBufferPtr( hBuf,(LPVOID*)&pBuffer));
				switchValue = pBuffer[samples-1];
				std::cout<<"switch: "<<switchValue<<std::endl;

				if (switchValue >= 45000)
				{
					if (readyFlag)
					{
						sprintf(inputChar,"DAQ started");
						send(comm->datasock, inputChar, sizeof(inputChar), 0);
					}
					//Start processing data out of pBuffer
					filtering = false;
					//sampels =2*sampling rate,
					for(int i=0;i<samples;i+2){
						value=pBuffer[i];
						volts = ((float)max-(float)min)/(1L<<resolution)*value +(float)min;
						rawSignal.push_back(volts);
					}
					std::vector<double>filteredSignal;
					filteredSignal=	filterSignal(rawSignal,filter,transientOverFlow,transientOverAmount);					
					//get min max etc.
					getStats(filteredSignal);

					//send signal to transmit convolved data
					sprintf(inputChar,"CompleteData");
					send(comm->datasock, inputChar, sizeof(inputChar), 0);

					//cast this garbage as a const char*
					std::stringstream strs;
					strs << filteredSignal.size();
				    std::string temp_str = strs.str();
					char const* pchar = temp_str.c_str();

					//send the size of the filteredSignal
					sprintf(inputChar,pchar);
					send(comm->datasock, inputChar, sizeof(inputChar), 0);

					//send the filtered signal
					for(int i=0;i<filteredSignal.size();i++){
						//convert
						std::stringstream strs;
						strs << filteredSignal[i];
						std::string temp_str = strs.str();
						char const* pchar = temp_str.c_str();
						
						sprintf(inputChar,pchar);
					    send(comm->datasock, inputChar, sizeof(inputChar), 0);
					}
					//send key word for min max
					sprintf(inputChar,"MINMAX");
					send(comm->datasock, inputChar, sizeof(inputChar), 0);

					//send the min/max etc.
					sprintf(inputChar,getStats(filteredSignal));
					send(comm->datasock, inputChar, sizeof(inputChar), 0);

					filtering = true;
					setLED(1);
					readyFlag = 0;
					processingStopped = 1;
				}
				else
				{
					
					setLED(0);
					if (processingStopped)
					{
						sprintf(myChar,"DAQ Stopped");
						std::cout<<"MyChar: "<<myChar<<std::endl;
						send(comm->datasock, myChar, sizeof(myChar), 0);
						readyFlag = 1;
						processingStopped = 0;
					}
					
				}
			}
			CHECKERROR(olDaPutBuffer((HDASS)hAD, hBuf));
			if (encoding != OL_ENC_BINARY)
			{
				value ^= 1L << (resolution-1);
				value &= (1L << resolution) - 1;
				switchValue^= 1L << (resolution-1);
				switchValue &= (1L << resolution) - 1;
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
		
	}
	std::cout<<"tests 1:"<<std::endl;
	return 0;
}


int main()
{    
  
  struct sp_struct profiler; 
  struct sockaddr_in saddr;
  struct hostent *hp;
  int res = 0;
  char ParamBuffer[300];
  
  memset(&profiler, 0, sizeof(profiler));
  comm = &profiler.comm;  

	if ((res = WSAStartup(0x202,&wsaData)) != 0)  {
		fprintf(stderr,"WSAStartup failed with error %d\n",res);
		WSACleanup();
		return(ERR_CODE_SWI);
	}

	
	/**********************************************************************************
	 * Setup data transmition socket to broadcast data
	 **********************************************************************************/

	hp = (struct hostent*)malloc(sizeof(struct hostent));
	hp->h_name = (char*)malloc(sizeof(char)*17);
	hp->h_addr_list = (char**)malloc(sizeof(char*)*2);
	hp->h_addr_list[0] = (char*)malloc(sizeof(char)*5);
	strcpy(hp->h_name, "lab_example\0");
	hp->h_addrtype = 2;
	hp->h_length = 4;

	//broadcast in 255.255.255.255 network 	

	hp->h_addr_list[0][0] = (signed char)255;//192;129
	hp->h_addr_list[0][1] = (signed char)255; //168;107



	hp->h_addr_list[0][2] = (signed char)255; //0;255  
	hp->h_addr_list[0][3] = (signed char)255; //140;255
	hp->h_addr_list[0][4] = 0;
    
	/**********************************************************************************
	 * Setup a socket for broadcasting data 
	 **********************************************************************************/
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = hp->h_addrtype;
	memcpy(&(saddr.sin_addr), hp->h_addr, hp->h_length);
	saddr.sin_port = htons(1500);
		
	if ((comm->datasock = socket(AF_INET,SOCK_DGRAM, 0)) == INVALID_SOCKET) {
	    fprintf(stderr,"socket(datasock) failed: %d\n",WSAGetLastError());
		WSACleanup();
		return(ERR_CODE_NONE);
	}

	if (connect(comm->datasock,(struct sockaddr*)&saddr,sizeof(saddr)) == SOCKET_ERROR) {
	    fprintf(stderr,"connect(datasock) failed: %d\n",WSAGetLastError());
		WSACleanup();
		return(ERR_CODE_SWI);
	}

    /**********************************************************************************
	 * Setup and bind a socket to listen for commands from client
	 **********************************************************************************/
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY; 
	saddr.sin_port = htons(1024);	
	if ((comm->cmdrecvsock = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
	    fprintf(stderr,"socket(cmdrecvsock) failed: %d\n",WSAGetLastError());
		WSACleanup();
		return(ERR_CODE_NONE);
	}	

	if (bind(comm->cmdrecvsock,(struct sockaddr*)&saddr,sizeof(saddr) ) == SOCKET_ERROR) {
		fprintf(stderr,"bind() failed: %d\n",WSAGetLastError());
		WSACleanup();
		return(ERR_CODE_NONE);
	}

	hClientThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) client_iface_thread, (LPVOID)&profiler, 0, &dwClientThreadID); 
	SetThreadPriority(hClientThread, THREAD_PRIORITY_LOWEST);
	
	while(1);

	return 0; 
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
		retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0,(struct sockaddr *)&saddr, &saddr_len);
		if(strcmp(ParamBuffer,"Start") == 0)
		{
			std::cout<<"Start Signal Received Waiting for Switch"<<std::endl;
			sprintf(inputChar,"Server: Start signal Received Waiting for Switch");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			//startDAQ();///////////////////---------------Reset in lab
			startSignal = true;
		}
		if(strcmp(ParamBuffer, "Stop") == 0)
		{
			setLED(0);
			std::cout<<"Stop Signal Received"<<std::endl;
			sprintf(inputChar,"Server: DAQ has Stopped");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			startSignal = false;
			
			/*sprintf(inputChar,"FileSave");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			FILE* fp = fopen("result.txt","r");
			while (fgets(buf,100, fp)!=NULL)
			{
				sprintf(inputChar,"%s",buf);
				send(comm->datasock, inputChar, sizeof(inputChar), 0);
				//sending convovled results to client
			}
			fclose(fp);
			sprintf(inputChar,"stop");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);*/
		}
		if(strcmp(ParamBuffer, "SampleRate") == 0)
		{
			retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
			sampleRate = strtod(ParamBuffer,NULL);
			samples=2*sampleRate;
			//set the sample rate	
			printf("Current Sample Rate: %f\n", sampleRate);
			sprintf(inputChar,"Server: Sample Rate Received by Server");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			//intilializeBoard();///////////-------------Reset in lab
			startSignal=true;
		}
		if(strcmp(ParamBuffer, "FilterWeights") == 0)
		{
			for(i=0;i<100;i++)
			{
				retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
				//std::cout<<"i: "<<i<<std::endl;
				filter.push_back( atof(ParamBuffer));
			}
			std::cout<<"Filter Received"<<std::endl;
			sprintf(inputChar,"Server: Filter Received by Server");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			transientOverAmount=(filter.size()-1);
		}
		if(strcmp(ParamBuffer, "FileSave") == 0)
		{
			retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
			printf("File name of Results: %s\n", ParamBuffer);
			sprintf(inputChar,"Server: File Name Received");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			sprintf(inputChar,"Server: Waiting for Start Command: Type Start continue DAQ");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			settings = true;
		}
	}
	x= 0;
}
VOID client_iface_sending_thread(LPVOID parameters) //LPVOID parameters)
{
	sp_struct_t profiler = (sp_struct_t)parameters;
	comm = &profiler->comm;
	INT retval;
	struct sockaddr_in saddr;
	int saddr_len;
	char inputChar[110] = {0};
}


void setLED(int level){
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
	CHECKERROR (olDaPutSingleValue(board.hdass,level,channel,gain));
	CHECKERROR (olDaReleaseDASS(board.hdass));
	CHECKERROR (olDaTerminate(board.hdrvr));

}


//set when start signal is received
int intilializeBoard(){

	WNDCLASS wc;
    memset( &wc, 0, sizeof(wc) );
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = "DtConsoleClass";
    RegisterClass( &wc );

    hWnd = CreateWindow( wc.lpszClassName,
                              NULL, 
                              NULL, 
                              0, 0, 0, 0,
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    if( !hWnd )
       exit( 1 );
	
    
	hDev = NULL;
    CHECKERROR( olDaEnumBoards( EnumBrdProc, (LPARAM)&hDev ) );

    hAD = NULL;
    CHECKERROR( olDaGetDASS( hDev, OLSS_AD, 0, &hAD ) ); 

    CHECKERROR( olDaSetWndHandle( hAD, hWnd, 0 ) ); 

    CHECKERROR( olDaSetDataFlow( hAD, OL_DF_CONTINUOUS ) ); 
    CHECKERROR( olDaSetChannelListSize( hAD, 2 ) );//changed to 2
    CHECKERROR( olDaSetChannelListEntry( hAD, 0, 0 ) );//changed to 1
	CHECKERROR( olDaSetChannelListEntry( hAD, 1, 1 ) );//changed to 1
    CHECKERROR( olDaSetGainListEntry( hAD, 0, 1 ) );
	CHECKERROR( olDaSetGainListEntry( hAD, 1, 1 ) );
    CHECKERROR( olDaSetTrigger( hAD, OL_TRG_SOFT ) );
    CHECKERROR( olDaSetClockSource( hAD, OL_CLK_INTERNAL ) ); 
	std::cout<<"sample rate set to:" <<sampleRate<<std::endl;
    CHECKERROR( olDaSetClockFrequency( hAD, sampleRate) ); //Set when SampleRate is recevied by client
    CHECKERROR( olDaSetWrapMode( hAD, OL_WRP_NONE ) );
    CHECKERROR( olDaConfig( hAD ) );
	std::cout<<"hAD: "<< hAD<<std::endl;

    
    for( int i=0; i < NUM_OL_BUFFERS; i++ )
    {
        if( OLSUCCESS != olDmAllocBuffer( GHND, 1000, &hBufs[i] ) )
        {
           for ( i--; i>=0; i-- )
           {
              olDmFreeBuffer( hBufs[i] );
           }   
           exit( 1 );   
        }
        olDaPutBuffer( hAD,hBufs[i] );
    }
}
//called when switch is pon and start signal is recevied

void startDAQ(){
	if( OLSUCCESS != ( olDaStart( hAD ) ) )
    {
       printf( "( .) ( . )A/D Operation Start Failed...hit any key to terminate.\n" );
    }
    else
    {
       printf( "A/D Operation Started...hit any key to terminate.\n\n" );
       printf( "Buffer Done Count : %ld \r", counter );
    }   

    MSG msg;                                    
    SetMessageQueue( 50 );                      // Increase the our message queue size so
                                                // we don't lose any data acq messages
 
    // Acquire and dispatch messages until a key is hit...since we are a console app 
    // we are using a mix of Windows messages for data acquistion and console approaches
    // for keyboard input.
    //
    while( GetMessage( &msg,        // message structure              
                       hWnd,        // handle of window receiving the message 
                       0,           // lowest message to examine          
                       0 ) )        // highest message to examine         
    {
       TranslateMessage( &msg );    // Translates virtual key codes       
       DispatchMessage( &msg );     // Dispatches message to window 
       if( _kbhit() )
       {
          _getch();
          PostQuitMessage(0);      
       }
    }

    // abort A/D operation
    olDaAbort( hAD );
    printf( "\nA/D Operation Terminated \n" );

    for( i=0; i<NUM_OL_BUFFERS; i++ ) 
    {
       olDmFreeBuffer( hBufs[i] );
    }   

    olDaTerminate( hDev );
    exit( 0 );
    printf( "Open Layers Continuous A/D Win32 Console Example\n" );

	
	while(x == 1 )
	{
		if( _kbhit() )
       {
		   scanf("%s", inputChar);		   		   
		   send(comm->datasock, inputChar, sizeof(inputChar), 0);
		}		
	}
	

}

//given the signal, the filter to apply, the TranseitnOverFlow from precious buffer and the size of that over flow: calculate real time convolution
//returns: signal modified by filter
//updates transientOverflow at Global scope
std::vector<double> filterSignal(std::vector<double> filter, std::vector<double> signal,std::vector<double> transientOverFlow,int transientOverAmount){
	
	//Multi block is calculated by col=signalLength+(FilterLength-1)	
	int columnLength=signal.size()+(filter.size()-1);
	//row is calculated by row=filterLength
	int rowLength=filter.size();	
	//filtered signal length=columnLength
	int filteredSignalSize= columnLength;
	std::vector<double> filteredSignal;
	filteredSignal.resize(filteredSignalSize);
	//resize signal for convolution
	std::cout<<"Signal original size"<<signal.size()<<std::endl;
	for(int i=signal.size();i<columnLength;i++){
		signal.push_back(0);
	}
	std::cout<<"new signal size: "<<signal.size()<<std::endl;
	
	//Resize colum anbd rows
	std::vector<std:: vector< double> > multiBlock(rowLength, std::vector<double> ( columnLength, 0 ));
	std::cout<<"Size of row: "<< multiBlock.size()<<"Size of Columns: "<< multiBlock[0].size()<<std::endl;
	
	//convolution	
	for(int k=0;k<filter.size();k++){
		for(int n=0;n<signal.size();n++){			
			if((n-k)>=0){
			    multiBlock[k][n]= filter[k]*signal[n-k];
			}
			
		}
	}
	
	/*save multiblock to CSV
	fstream f;
	f.open("multiBlock.csv", ios::out);
	for(int k=0;k<filter.size();k++){
		for(int n=0;n<columnLength;n++){			
			    f<<multiBlock[k][n]<<',';			
		}
		f<<endl;
	}*/
	//add up the columns
	for(int i=0;i<columnLength;i++){
		for(int j=0;j<filter.size();j++){
			filteredSignal[i]= filteredSignal[i]+multiBlock[j][i];
		}
	}
	//set the transient overflow to current filter
	for(int i=0;i<transientOverFlow.size();i++){
		std::cout<<"filter val: "<<filteredSignal[i]<< " + "<<transientOverFlow[i]<<std::endl;
		filteredSignal[i]+=transientOverFlow[i];
		std::cout<<"filterSignal: "<<filteredSignal[i]<<std::endl;
	}
	
	//print out the filtered signal
	std::cout<<"Filtered signal: "<<std::endl;
	for(int i=0;i<filteredSignal.size();i++){
		std::cout<<filteredSignal[i]<<std::endl;
	}
	
	//return to transient region
	int j=0;
	for(int i=(filteredSignal.size()-transientOverAmount);i<filteredSignal.size();i++){		
		transientOverFlow[j]=filteredSignal[i];
		j++;
	}
	return filteredSignal;
}
//helper methods that gets MAX/MIN/VAR/AVG
const char* getStats(std::vector<double>filteredSignal){
	std::ostringstream strs;
	strs<<getAverage(filteredSignal);
	std::string avgString=strs.str();

	std::string myString="Max: "+getMax(filteredSignal)+" Min: "+getMin(filteredSignal)+" Avg: "+avgString+" Var: "+getVariance(filteredSignal);
	const char* returnChar=myString.c_str();
	return returnChar;
}
//may have to chop off the transient area to get correct MIN MAX AVG etc. use transientOverAmount to chopp
std::string getMax(std::vector<double>filteredSignal){
	double max=0;
	std::ostringstream strs;
	for(int i=0;i<filteredSignal.size();i++){
		if(filteredSignal[i]>max){
			max=filteredSignal[i];
		}
	}
	strs<<max;
	return strs.str();
}

std::string getMin(std::vector<double>filteredSignal){
	double min=filteredSignal[0];

	std::ostringstream strs;
	for(int i=0;i<filteredSignal.size();i++){
		if(filteredSignal[i]<min){
			min=filteredSignal[i];
		}
	}
	strs<<min;
	return strs.str();
}

double getAverage(std::vector<double>filteredSignal){
	double average=0;
	double sum=0;

	for(int i=0;i<filteredSignal.size(); i++)
	{
		sum+= filteredSignal[i];
	}
	average=(sum/filteredSignal.size());

	
	return average;
}

std::string getVariance(std::vector<double>filteredSignal){
	double sum=0;
	double variance=0;
	double average=0;
	std::ostringstream strs;
	
	average = getAverage(filteredSignal);
	sum=0;
	for(int i=0;i<filteredSignal.size(); i++)
	{
		sum= sum + pow((filteredSignal[i] - average), 2);
	}
	variance = sum/filteredSignal.size();
         
	strs<<variance;
	return strs.str();
}