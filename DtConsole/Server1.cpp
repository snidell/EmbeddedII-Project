/*-----------------------------------------------------------------------

PROGRAM: DtConsole.cpp

PURPOSE:
    Open Layers data acquisition example showing how to implement a 
    continuous analog input operation using windows messaging in a 
    console environment.
    
****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h> //for atof
#include <conio.h>
#include "oldaapi.h"
#include <olmem.h> 
#include <olerrors.h>  
#include "SocketStructure.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
using namespace std;
double sampleRate;
UINT channel = 0;//channel for LED
DBL gain = 1.0;
UINT encoding=0,resolution=0;
ULNG samples;
vector<double> filterWeights;
int switch_state=0;
#define STRLEN 80        /* string size for general text manipulation   */
char str[STRLEN];        /* global string for general text manipulation */
char inputChar[300] = "";
static BOARD board;
bool filteringOn=false;
int started=1;
int stopped=1;
string saveFileName;
bool readingSignal=false;
bool settings=false;


vector<double> filterSignal(vector<double> filter, vector<double> signal);
int saveToCSV(const string& filename, vector<double> signal);
void setLED(int level);
int getLength(char*ParamBuffer);
bool sampleData(sp_comm_t comm);
string getMax(vector<double>filteredSignal);
string getMin(vector<double>filteredSignal);
string getVariance(vector<double>filteredSignal);
string getAverage(vector<double>filteredSignal);


#define SHOW_ERROR(ecode) MessageBox(HWND_DESKTOP,olDaGetErrorString(ecode,\
                  str,STRLEN),"Error", MB_ICONEXCLAMATION | MB_OK);

#define CHECKERROR( ecode )                           \
do                                                    \
{                                                     \
   ECODE olStatus;                                    \
   if( OLSUCCESS != ( olStatus = ( ecode ) ) )        \
   {                                                  \
      printf("OpenLayers Error %d\n", olStatus );     \
      system("pause");                                \
      exit(1);                                        \
   }                                                  \
}                                                     \
while(0)     
#define CLOSEONERROR(ecode) if ((board.status = (ecode)) != OLNOERROR)\
                  {\
                  SHOW_ERROR(board.status);\
                  olDaReleaseDASS(board.hdass);\
                  olDaTerminate(board.hdrvr);\
                  return (TRUE);}

#define NUM_OL_BUFFERS 4

int counter = 0;

LRESULT WINAPI 
WndProc( HWND hWnd, UINT msg, WPARAM hAD, LPARAM lParam )
{
    switch( msg )
    {
       case OLDA_WM_BUFFER_DONE:
          printf( "Buffer Done Count : %ld \r", counter );
          HBUF hBuf;
          counter++;
          olDaGetBuffer( (HDASS)hAD, &hBuf );
          olDaPutBuffer( (HDASS)hAD, hBuf );
		  //do all processing here
		  if( hBuf )
          {
			 
          }
         return (TRUE);   /* Did process a message */
          break;

       case OLDA_WM_QUEUE_DONE:
          printf( "\nAcquisition stopped, rate too fast for current options." );
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
    
    return 0;
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

void intializeBoard(double sampleFreq){	
	
    //while(!settings);
	//while(!readingSignal);
	HDEV hDev = NULL;
    CHECKERROR( olDaEnumBoards( EnumBrdProc, (LPARAM)&hDev ) );
	
    HDASS hAD = NULL;
    CHECKERROR( olDaGetDASS( hDev, OLSS_AD, 0, &hAD ) ); 
    //CHECKERROR( olDaSetWndHandle( hAD, hWnd, 0 ) ); 
	
    CHECKERROR( olDaSetDataFlow( hAD, OL_DF_CONTINUOUS ) ); 
    CHECKERROR( olDaSetChannelListSize( hAD, 1 ) );
    CHECKERROR( olDaSetChannelListEntry( hAD, 0, 0 ) );
    CHECKERROR( olDaSetGainListEntry( hAD, 0, 1 ) );
    CHECKERROR( olDaSetTrigger( hAD, OL_TRG_SOFT ) );
    CHECKERROR( olDaSetClockSource( hAD, OL_CLK_INTERNAL ) ); 

	//sample frequency
    CHECKERROR( olDaSetClockFrequency( hAD, sampleFreq) );
    CHECKERROR( olDaSetWrapMode( hAD, OL_WRP_NONE ) );
    CHECKERROR( olDaConfig( hAD ) );
	
    HBUF hBufs[NUM_OL_BUFFERS];
    for( int i=0; i < NUM_OL_BUFFERS; i++ )
    {
        if( OLSUCCESS != olDmAllocBuffer( GHND, 1000, &hBufs[i] ) )
        {
           for ( i--; i>=0; i-- )
           {
              olDmFreeBuffer( hBufs[i] );
           }   
          // exit( 1 );   
        }
        olDaPutBuffer( hAD,hBufs[i] );
    }
	
    if( OLSUCCESS != ( olDaStart( hAD ) ) )
    {
       printf( "A/D Operation Start Failed...hit any key to terminate.\n" );
    }
    else
    {
       printf( "A/D Operation Started...hit any key to terminate.\n\n" );
       printf( "Buffer Done Count : %ld \r", counter );
    }   

    // MSG msg;                                    
    //SetMessageQueue( 50 );                      // Increase the our message queue size so
                                               // we don't lose any data acq messages    

    // abort A/D operation
    //olDaAbort( hAD );
    //printf( "\nA/D Operation Terminated \n" );
	/*
    for(int i=0; i<NUM_OL_BUFFERS; i++ ) 
    {
       olDmFreeBuffer( hBufs[i] );
    }   

    olDaTerminate( hDev );
	
    //exit( 0 );*/
	

}

int main()
{
  printf( "Open Layers Continuous A/D Win32 Console Example\n" );
  struct sp_struct profiler; 
  struct sockaddr_in saddr;
  struct hostent *hp;
  int res = 0;
  char ParamBuffer[300];
  
  memset(&profiler, 0, sizeof(profiler));
  sp_comm_t comm = &profiler.comm;  

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
	
	while(x == 1 )
	{
		if( _kbhit() )
       {
		   scanf("%s", inputChar);		   		   
		   send(comm->datasock, inputChar, sizeof(inputChar), 0);
		}		
	}
	while(1);
	return 0; 

}

/************************************************************************************
 * VOID client_iface_thread(LPVOID)
 *
 * Description: Thread communicating commands from client and the status of their 
 *				completion back to client. 
 * 
 *
 ************************************************************************************/
VOID client_iface_thread(LPVOID parameters) //LPVOID parameters
{
  sp_struct_t profiler = (sp_struct_t)parameters;
  sp_comm_t comm = &profiler->comm; 
  INT retval;
  struct sockaddr_in saddr;
  int saddr_len;
  char ParamBuffer[300]="" ;

    printf("Executing Thread\n");
	printf("Checking for Data\n");
	while(ParamBuffer[0] != '!') 
	{
		memset(ParamBuffer, 0, sizeof(ParamBuffer));
		saddr_len =sizeof(saddr);		
		retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len); 
		//-----------------------------------------------------------------------------------------------------
		if(strcmp(ParamBuffer,"FilterWeights")==0){
			retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
			int length=getLength(ParamBuffer);
			
			cout<<"lengths is: "<< length<<endl;
			int i;
			for(i=0;i<length;i++){
				retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
				filterWeights.push_back(atof(ParamBuffer));
				cout<<" count: "<<i<<endl;
			}
			cout<<"final i: "<<i<<endl;
			for(int i =0;i<filterWeights.size();i++){
				cout<<filterWeights[i]<<endl;
			}
		}		
		//-----------------------------------------------------------------------------------------------------
		if(strcmp(ParamBuffer,"SampleRate")==0){
			
			retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
			cout<<"Sample rate recevied"<<endl;
			sprintf(inputChar,"SampleRecevied");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			//set the sample rate in DT9816
			sampleRate=atof(ParamBuffer);					
			//initialize AD conversion
				

			//switch off end samping data
			cout<<"Sample rate set to: "<<sampleRate<<endl;
		}
		//-----------------------------------------------------------------------------------------------------
		if(strcmp(ParamBuffer,"FileName")==0){
			retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
			saveFileName=ParamBuffer;
			sprintf(inputChar,"FileReceived");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			//all settings received GTG
			settings=true;
		}
		//-----------------------------------------------------------------------------------------------------
		cout<<"Stirng comp Start: "<< (strcmp(ParamBuffer,"Start")==0)<<endl;
		if(strcmp(ParamBuffer,"Start")==0){
			sprintf(inputChar,"Started");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			readingSignal=true;
			intializeBoard(sampleRate);	
			sampleData(comm);
			
		}
		//-----------------------------------------------------------------------------------------------------
		if(strcmp(ParamBuffer,"Stop")==0){
			sprintf(inputChar,"Stopped");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			setLED(0);
			readingSignal=false;
			sprintf(inputChar,"SaveFile");
			send(comm->datasock, inputChar, sizeof(inputChar), 0);
			//need to send file back over to client wtf

		}
				
	}
	x = 0;
}

vector<double> filterSignal(vector<double> filter, vector<double> signal){
	//Multi block is calculated by col=signalLength+(FilterLength-1)	
	int columnLength=signal.size()+(filter.size()-1);
	//row is calculated by row=filterLength
	int rowLength=filter.size();	
	//filtered signal length=columnLength
	int filteredSignalSize= columnLength;
	vector<double> filteredSignal;
	filteredSignal.resize(filteredSignalSize);
	//resize signal for convolution
	cout<<"Signal original size"<<signal.size()<<endl;
	for(int i=signal.size();i<columnLength;i++){
		signal.push_back(0);
	}
	cout<<"new signal sie: "<<signal.size()<<endl;
	
	//Resize colum anbd rows
	vector< vector< double> > multiBlock(rowLength, vector<double> ( columnLength, 0 ));
	cout<<"Size of row: "<< multiBlock.size()<<"Size of Columns: "<< multiBlock[0].size()<<endl;
	//convolution
	

	
	for(int k=0;k<filter.size();k++){
		for(int n=0;n<signal.size();n++){			
			if((n-k)>=0){
			    multiBlock[k][n]= filter[k]*signal[n-k];
			}
			
		}
	}
	
	//save multiblock to CSV
	fstream f;
	f.open("multiBlock.csv", ios::out);
	for(int k=0;k<filter.size();k++){
		for(int n=0;n<columnLength;n++){			
			    f<<multiBlock[k][n]<<',';			
		}
		f<<endl;
	}
	
	for(int i=0;i<columnLength;i++){
		for(int j=0;j<filter.size();j++){
			filteredSignal[i]= filteredSignal[i]+multiBlock[j][i];
		}
	}

	

	
	
	return filteredSignal;
}

int saveToCSV(const string& filename, vector<double> signal){
	fstream f;
    f.open(filename, ios::out);
    for(int i=0; i<signal.size(); i++){
		if(i==signal.size()-1){
			f<<signal[i];
		}else{			
			f << signal[i]<<',';
	    }		
	}     
    f.close();
	cout<<"Size of signal: "<<signal.size()<<endl;
    return 0;

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


int getLength(char*ParamBuffer){	
	int length=int(ParamBuffer[0]);
	cout<<"Length of vector is: "<<length<<endl;
	return length;
}

bool sampleData(sp_comm_t comm){
	DASSINFO ssinfo;
	char temp[128];
	HBUF  hBuffer = NULL;
	PDWORD  pBuffer32 = NULL;
    PWORD  pBuffer = NULL;
	DBL volts, volts2;
    ULNG value, switchValue;
	DBL min=0,max=0;
	int startFlag=1;
	vector<double> buffer;
	vector<double>filteredSignal;
	string statistics;
	olDaGetDASSInfo(board.hdass,&ssinfo);//do i need this?
	itoa(ssinfo.uiElement,temp,10);//do i need this?


	/* get buffer off done list */
	    cout<<"Check -1"<<endl;
         CHECKERROR (olDaGetBuffer(board.hdass, &hBuffer));
		 cout<<"Check 1"<<endl;

         /* if there is a buffer */

         if( hBuffer&&readingSignal&&settings)
         {

             /* get sub system information for code/volts conversion */

             CLOSEONERROR (olDaGetRange(board.hdass,&max,&min));
			 cout<<"Check 2"<<endl;
             CLOSEONERROR (olDaGetEncoding(board.hdass,&encoding));
			 cout<<"Check 3"<<endl;
             CLOSEONERROR (olDaGetResolution(board.hdass,&resolution));
			 cout<<"Check 4"<<endl;

            /* get max samples in input buffer */

            CLOSEONERROR (olDmGetValidSamples( hBuffer, &samples ) );
			cout<<"Check 5"<<endl;
            /* get pointer to the buffer */
            if (resolution > 16)
            {
				cout<<"Check 6"<<endl;
				CLOSEONERROR (olDmGetBufferPtr( hBuffer,(LPVOID*)&pBuffer32));
				cout<<"Check 7"<<endl;
				/* get last sample in buffer */
				switchValue = pBuffer32[samples-1];
            }
            else
            {
				cout<<"Check 8"<<endl;
				CLOSEONERROR (olDmGetBufferPtr( hBuffer,(LPVOID*)&pBuffer));
				cout<<"Check 9"<<endl;
				switchValue = pBuffer[samples-1];
				if(switchValue>=49000)
				{
					
					if(started)
					{
						sprintf(inputChar,"Data Aquistion has Started");
						send(comm->datasock, inputChar, sizeof(inputChar), 0);

					}
					filteringOn =false;

					for(int i=0;i<samples;i=i+2){
						value=pBuffer[i];
						volts = ((float)max-(float)min)/(1L<<resolution)*value+(float)min;
						buffer.push_back(volts);
					}
					filteredSignal=filterSignal(filterWeights,buffer);
					//save to file
					statistics="Max: "+getMax(buffer)+" Min: "+getMin(buffer)+" Average: "+getAverage(buffer)+" Variance: "+getVariance(buffer);
					sprintf(inputChar,statistics.c_str());
					send(comm->datasock, inputChar, sizeof(inputChar), 0);
					filteringOn=true;
					setLED(1);
					started=0;
					stopped=1;
				}
				else
				{
					setLED(3);
					if(stopped){
						sprintf(inputChar,"Data Aquistion Stopped");
						send(comm->datasock, inputChar, sizeof(inputChar), 0);
						started=1;
						stopped=0;						
					}
				}
            }
            /* put buffer back to ready list */
			cout<<"Check 10"<<endl;
            CHECKERROR (olDaPutBuffer(board.hdass, hBuffer));
			cout<<"Check 11"<<endl;
            /*  convert value to volts */

            if (encoding != OL_ENC_BINARY) 
            {
               /* convert to offset binary by inverting the sign bit */
      
               value ^= 1L << (resolution-1);
               value &= (1L << resolution) - 1;     /* zero upper bits */
			   switchValue^= 1L << (resolution-1);
			   switchValue&= (1L << resolution) - 1;

            }   
		 }
}

string getMax(vector<double>filteredSignal){
	double max=0;
	ostringstream strs;
	for(int i=0;i<filteredSignal.size();i++){
		if(filteredSignal[i]>max){
			max=filteredSignal[i];
		}
	}
	strs<<max;
	return strs.str();
}

string getMin(vector<double>filteredSignal){
	double min=filteredSignal[0];

	ostringstream strs;
	for(int i=0;i<filteredSignal.size();i++){
		if(filteredSignal[i]<min){
			min=filteredSignal[i];
		}
	}
	strs<<min;
	return strs.str();
}

string getVariance(vector<double>filteredSignal){
	double sum=0;
	double variance=0;
	double average=0;
	ostringstream strs;

	for(int i=0;i<filteredSignal.size(); i++)
	{
		sum += filteredSignal[i];
	}
	average = sum/filteredSignal.size();
	sum=0;
	for(int i=0;i<filteredSignal.size(); i++)
	{
		sum= sum + pow((filteredSignal[i] - average), 2);
	}
	variance = sum/filteredSignal.size();
         
	strs<<variance;
	return strs.str();
}

string getAverage(vector<double>filteredSignal){
	double average=0;
	double sum=0;
	ostringstream strs;

	for(int i=0;i<filteredSignal.size(); i++)
	{
		sum+= filteredSignal[i];
	}
	average=(sum/filteredSignal.size());

	strs<<average;
	return strs.str();
}