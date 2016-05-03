#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "SocketStructure.h"
using namespace std;
vector<double> readSignal(const string& filename);
void createFile(const string& filename);
int saveToCSV(const std::string& filename, std::vector<double> signal);
char fileName[200];
int main()
{
  vector<double>filterWeights;
  string filterFileName="";
  
  char* sampleRate="";
  //---Above was added for project
  struct sp_struct profiler; 
  struct sockaddr_in saddr;
  struct hostent *hp;
  int res = 0;
  char ParamBuffer[110];
  
  

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
	hp->h_addr_list[0][2] = (signed char)255; //0; 255
	hp->h_addr_list[0][3] = (signed char)255; //140;255
	hp->h_addr_list[0][4] = 0;

	/**********************************************************************************
	 * Setup a socket for broadcasting data 
	 **********************************************************************************/
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = hp->h_addrtype;
	memcpy(&(saddr.sin_addr), hp->h_addr, hp->h_length);
	saddr.sin_port = htons(1024);
		
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
	saddr.sin_port = htons(1500);	
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
	
	while(x == 1)
	{
		if( _kbhit() )
       {		   
		   //------------------------------------------------------	----
		   //cout<<"At which rate do you wish to sample? (Hz)"<<endl;
		   sprintf(inputChar,"SampleRate");
		   send(comm->datasock, inputChar, sizeof(inputChar), 0);
		   cin>>inputChar;
		   send(comm->datasock, inputChar, sizeof(inputChar), 0);
		   Sleep(10);
		   
		   //----------------------------------------------------------
		   cout<<"What is name of the File for filter weights?"<<endl;
		   cin>>filterFileName;
		   filterWeights=readSignal(filterFileName);
		   cout<<"Reading file: "<<filterFileName<<endl;

		   sprintf(inputChar,"FilterWeights");
		   send(comm->datasock, inputChar, sizeof(inputChar), 0);

		   //int size=filterWeights.size();
		   //cout<<"length Being sent: "<< size<<endl;
		   //memcpy(inputChar, (char*)&size,100);
		   //cout<<"char rep: "<<inputChar<<endl;

		   //send(comm->datasock, inputChar, sizeof(inputChar), 0);

		   for(int i=0;i<filterWeights.size();i++){
			   sprintf(inputChar,"%f",filterWeights[i]);
			   send(comm->datasock, inputChar, sizeof(inputChar), 0);
			   Sleep(1);
		   }
		   //----------------------------------------------------------
		   cout<<"What is the name of the file to save data to? Note, this is saved as a CSV file"<<endl;
		   cin>>fileName;
		   sprintf(inputChar,"FileSave");
		   send(comm->datasock, inputChar, sizeof(inputChar), 0);
		   cout<<"Sending File Name that results are stored in"<<endl;
		   sprintf(inputChar,fileName);
		   send(comm->datasock, inputChar, sizeof(inputChar), 0);
		   Sleep(1);
		   
		   //----------------------------------------------------------
		   while(1){
			   string promptStart="";
			   //cout<<"Waiting for Start signal type Start to intialize DAQ type Stop to stop DAQ"<<endl;
		       cin>>promptStart;
			   if(promptStart.compare("Start")==0){
				   sprintf(inputChar,"Start");
				   send(comm->datasock, inputChar, sizeof(inputChar), 0);
			   }else if(promptStart.compare("Stop")==0){
				   sprintf(inputChar,"Stop");
				   send(comm->datasock, inputChar, sizeof(inputChar), 0);
			   }else{
				   cout<<"Incorrect code, try again"<<endl;
			   }
		   }	   
		}		
	}

	

	return 0 ; 
}

/************************************************************************************
 * VOID client_iface_thread(LPVOID)
 *
 * Description: Thread communicating commands from client and the status of their 
 *				completion back to client. 
 * 
 *
 ************************************************************************************/
VOID client_iface_thread(LPVOID parameters) //LPVOID parameters)
{
  sp_struct_t profiler = (sp_struct_t)parameters;
  sp_comm_t comm = &profiler->comm; 
  INT retval;
  struct sockaddr_in saddr;
  int saddr_len;  

  cout<<"At which rate do you wish to sample? (Hz)"<<endl;
	while(ParamBuffer[0] != '!') 
	{
		memset(ParamBuffer, 0, sizeof(ParamBuffer));
		saddr_len =sizeof(saddr);		
		retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);   
		if(strcmp(ParamBuffer,"FileReceived")==0){
			cout<<"File Name Received by Server"<<endl;
		}

		if(strcmp(ParamBuffer,"SampleRecevied")==0){
			cout<<"Sample Rate Recevied by Server"<<endl;
		}

		if(strcmp(ParamBuffer,"Started")==0){
			cout<<"Data Aquisition Started"<<endl;
		}
		if(strcmp(ParamBuffer,"Stopped")==0){
			cout<<"Data Aquisition Stopped"<<endl;
		}
		//NEED TO TEST ---------------------------------------------------------------------------
		if(strcmp(ParamBuffer,"CompleteData")==0){
			cout<<"Server: data Being sent..."<<endl;
			retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
			//getting size of vector
			int size=atoi(ParamBuffer);
			std::cout<<"size: "<<size<<std::endl;
			//getting data in that vector
			std::vector<double> myDouble;
			for(int i=0;i<size;i++){
				retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
				std::cout<<"Parambuff: "<<i<<" "<<ParamBuffer<<std::endl;
				myDouble.push_back(atof(ParamBuffer));
			}
			//verifying data
			std::cout<<"printing myDouble Vector"<<std::endl;
			for(int i=0;i<myDouble.size();i++){
				std::cout<<myDouble[i]<<std::endl;
			}
			//saveData to file
			saveToCSV(fileName,myDouble);
		}
		//--------------------NEED TO TEST----------------------------------------------------
		if(strcmp(ParamBuffer,"MINMAX")==0){
			retval = recvfrom(comm->cmdrecvsock, ParamBuffer, sizeof(ParamBuffer), 0, (struct sockaddr *)&saddr, &saddr_len);
			std::fstream f;
			f.open("MIN-MAX.txt", std::ios::out);
			f<<ParamBuffer;
			f.close();
		}
		cout<<ParamBuffer<<endl;
	}
	x = 0;
}


vector<double> readSignal(const string& filename){
	ifstream ifile(filename, ios::in);

	vector<double> dataRead;
	if (!ifile.is_open()) {
        cerr << "There was a problem opening the input file!\n";
    }
	
	double num = 0.0;
    //keep storing values from the text file so long as data exists:
    while (ifile >> num) {
        dataRead.push_back(num);
    }	
	
	return dataRead;
}

void createFile(const string& filename){
  ofstream myfile;
  myfile.open (filename);
  myfile << "File Intilized\n";
  myfile.close();
}

int saveToCSV(const std::string& filename, std::vector<double> signal){
	//requires fstream
	std::fstream f;
    f.open(filename, std::ios::out);
    for(int i=0; i<signal.size(); i++){
		if(i==signal.size()-1){
			f<<signal[i];
		}else{			
			f << signal[i]<<',';
	    }		
	}     
    f.close();
	std::cout<<"Size of signal: "<<signal.size()<<std::endl;
    return 0;

}