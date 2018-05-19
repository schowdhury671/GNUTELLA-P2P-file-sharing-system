//-------------------------------------------------------------
// @ Sanjoy Chowdhury - 20172123  Client 
//-------------------------------------------------------------

#include<iostream>
#include<fstream>
#include<cstdio>
#include<cstdlib>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sstream>
#include<netdb.h>
#include<errno.h>
#include<signal.h>
#include<cstring>
#include<string>
#include<vector>
#include<tuple>
#include<ctime>

#define MAXSIZE 1000000
#define MAXFILE 256
#define MAXPATH 4096
#define LQUEUE 5
#define NOT_ENOUGH_ARGS 1111
#define SOCKET_ERROR 1113
#define INV_ADDR 1114
#define CONNECT_ERROR 1115
#define INV_CHOICE 1116
#define RECV_ERROR 1117
#define INV_CONN 1118
#define BIND_ERROR 1119
#define SEND_ERROR 1011
#define IP_ERROR 1111

using namespace std;

ofstream fout("client_log.txt",ios::out|ios::trunc);
char *timestamp;
time_t cur;

class Client{

private:

	int sock_down_serv,sock_remote_client,sock_remote_serv,sock_remote_req;
	pid_t client_pid;
	struct sockaddr_in addr_down_serv,addr_remote_client,addr_remote_serv,addr_remote_req;
	int port_down_serv;

public:

	Client(){
		sock_down_serv=sock_remote_client=sock_remote_serv=0;
		memset(&addr_down_serv,0,sizeof(addr_down_serv));
		memset(&addr_remote_client,0,sizeof(addr_remote_client));
		memset(&addr_remote_serv,0,sizeof(addr_remote_serv));
		port_down_serv=0;
	}

//-------------------------------------------------------------
	//graceful shut_down
	void shut_down(){
		close(sock_down_serv);
		close(sock_remote_client);
		close(sock_remote_serv);
		exit(0);
	}

//-------------------------------------------------------------
	//error handling funtion 
	void handle_error(int errsv){
		cerr<<"Error: ";
		timestamp=ctime(&cur);
		fout<<timestamp<<"Error: ";
		if(errsv==EACCES){
			cerr<<"file could not be accessed - does not exist or invalid permissions"<<endl;
			fout<<"file could not be accessed - does not exist or invalid permissions"<<endl;
		}	
		else if(errsv==NOT_ENOUGH_ARGS){
			cerr<<"please specify [SERVER_PORT]"<<endl;
			fout<<"please specify [SERVER_PORT]"<<endl;
		}
		else if(errsv==SOCKET_ERROR){
			cerr<<"socket could not be established"<<endl;
			fout<<"socket could not be established"<<endl;
		}
		else if(errsv==INV_ADDR){
			cerr<<"invalid IP address entered"<<endl;
			fout<<"invalid IP address entered"<<endl;
		}
		else if(errsv==CONNECT_ERROR){
			cerr<<"socket could not connect to remote host"<<endl;
			fout<<"socket could not connect to remote host"<<endl;
		}
		else if(errsv==INV_CHOICE){
			cerr<<"invalid choice. terminating...."<<endl;
			fout<<"invalid choice. terminating...."<<endl;
		}
		else if(errsv==SEND_ERROR){
			cerr<<"could not send to remote host"<<endl;
			fout<<"could not send to remote host"<<endl;
		}
		else if(errsv==RECV_ERROR){
			cerr<<"could not receive from remote host"<<endl;
			fout<<"could not receive from remote host"<<endl;
		}
		else if(errsv==INV_CONN){
			cerr<<"socket could not accept connection"<<endl;
			fout<<"socket could not accept connection"<<endl;
		}
		else if(errsv==BIND_ERROR){
			cerr<<"socket could not be bound"<<endl;
			fout<<"socket could not be bound"<<endl;
		}
		else if(errsv==IP_ERROR){
			cerr<<"invalid IP address received"<<endl;
			fout<<"invalid IP address received"<<endl;
		}
		shut_down();
	}

//-------------------------------------------------------------
	//function handling writing of the received file at the client side
	void download(string filename,string filepath, string ip_address){
		ssize_t n;
		if((sock_remote_req=socket(AF_INET,SOCK_STREAM,0))<0)
			handle_error(SOCKET_ERROR);
		if(inet_pton(AF_INET,ip_address.c_str(),&addr_remote_req.sin_addr)<=0)
			handle_error(INV_ADDR);
		addr_remote_req.sin_family=AF_INET;
		addr_remote_req.sin_port=htons(port_down_serv);
		if(connect(sock_remote_req,(struct sockaddr *) &addr_remote_req,sizeof(addr_remote_req))<0)
			handle_error(CONNECT_ERROR);
		timestamp=ctime(&cur);
		fout<<timestamp<<"DOWNLOAD INITIATED!"<<endl;
		cout<<"DOWNLOAD INITIATED!"<<endl;
		char buffer[MAXSIZE];
		strcat(buffer,filepath.c_str());	
		if((n=write(sock_remote_req,buffer,MAXSIZE))<0)     //send file path to be downloaded from
			handle_error(SEND_ERROR);
		int file_fd=open(filename.c_str(),O_CREAT|O_RDWR|O_TRUNC,S_IRWXU);
		if(file_fd==-1){
			int errsv=errno;
			handle_error(errsv);
		}
		memset(buffer,0,sizeof(buffer));
		while((n=read(sock_remote_req,buffer,MAXSIZE))>0){		//receive incoming file
			char *temp_pointer=buffer;
			while(n>0){
				int sent=write(file_fd,temp_pointer,n);
				n-=sent; temp_pointer+=sent;
			}
		}
		close(sock_remote_req);
		timestamp=ctime(&cur);
		fout<<timestamp<<"DOWNLOAD COMPLETED!"<<endl;
		cout<<"DOWNLOAD COMPLETED!"<<endl;
	}

//-------------------------------------------------------------
	//function to search for a user specified file and to consequently initiate download request
	void search(){
		char filename[MAXFILE],message[MAXSIZE];
		cout<<endl<<"Enter filename to search: "<<endl;
		cout<<">> ";
		cin.getline(filename,MAXFILE);
		snprintf(message,MAXSIZE,"%s%s","search*_#",filename);
		unsigned int ch;
		if(write(sock_remote_serv,message,strlen(message)+1)<0)
			handle_error(SEND_ERROR);
		ssize_t n;
		memset(message,0,sizeof(message));
		if((n=read(sock_remote_serv,message,MAXSIZE))<0)		//retrieve mirrors
			handle_error(SEND_ERROR);
		if(n<=1)
			cout<<"No hits!"<<endl;
		else{
			cout<<endl<<"Search hits found!"<<endl;
			cout<<endl<<"Select a mirror: "<<endl;   
			char *token=strtok(message,"#");
			vector< tuple<string,string,string> > v;
			while(token!=NULL){
				istringstream temp((string(token)));    //parsing list of mirrors
				string a,b,c;
				getline(temp,a,':');
				getline(temp,b,':');
				getline(temp,c,':');
				v.push_back(make_tuple(a,b,c));
				token=strtok(NULL,"#");
			}
			int i=1;
			for(auto it=v.begin();it!=v.end();it++,i++)
				cout<<i<<". "<<get<0>(*it)<<"\t"<<get<2>(*it)<<endl;
			cout<<">> ";
			cin>>ch;
			cin.ignore();
			if(ch>0 && ch<=v.size())	
				download(get<0>(v[ch-1]),get<1>(v[ch-1]),get<2>(v[ch-1]));
			else
				handle_error(INV_CHOICE);
		}
	}
			
//-------------------------------------------------------------
	//function to share a file path with the server
	void share(){
		timestamp=ctime(&cur);
		char path[MAXPATH],*abs_path=NULL,message[MAXSIZE];
		cout<<endl<<"Enter file path: "<<endl;
		cout<<">> ";
		cin.getline(path,MAXPATH);
		if(access(path,F_OK|R_OK)==-1){  //check for file existence and valid user privileges
			int errsv=errno;
			handle_error(errsv);
		}
		if((abs_path=realpath(path,NULL))==NULL){		//convert relative path to absolute path
			int errsv=errno;
			handle_error(errsv);
		}
		char *filename=strrchr(abs_path,'/');
		snprintf(message,MAXSIZE,"%s#%s#%s","share*_",filename+1,abs_path);
		int bytes_to_be_sent=strlen(message)+1;
		if(write(sock_remote_serv,message,bytes_to_be_sent)<0)
			handle_error(SEND_ERROR);
		cout<<endl<<"File shared successfully!"<<endl;
		fout<<timestamp<<"File "<<string(filename+1)<<" shared successfully!"<<endl;
		free(abs_path);
	} 
		
//-------------------------------------------------------------
	//menu of features available to the user
	void menu(char* remote_address,char *remote_port){
		int ch;
		do{
			cout<<endl<<"--------------------------------------------------------------"<<endl;
			cout<<"Selection menu: "<<endl;
			cout<<"1. Search"<<endl;
			cout<<"2. Share"<<endl;
			cout<<"3. Exit"<<endl;
			cout<<">> ";
			cin>>ch;
			cin.ignore();
			string input;
			timestamp=ctime(&cur);
			switch(ch){
				case 1:	client_init(remote_address,remote_port);
								search();
								break;
				case 2:	client_init(remote_address,remote_port);
								share();
								break;
				case 3: fout<<timestamp<<"Shutting down client interface..."<<endl;
								cout<<endl<<"Shutting down client interface........"<<endl;
								break;
				default: handle_error(INV_CHOICE);
			}
			cout<<endl;
			close(sock_remote_serv);
		}while(ch!=3);
	}

//-------------------------------------------------------------
	//handle download request from remote client
	void serve_request(){
		char buffer[MAXSIZE];
		ssize_t n;
		if((n=read(sock_remote_client,buffer,MAXSIZE))<0)
			handle_error(RECV_ERROR);
		int file_fd=open(buffer,O_RDONLY);
		if(file_fd==-1){
			int errsv=errno;
			handle_error(errsv);
		}
		memset(buffer,0,sizeof(buffer));
		while((n=read(file_fd,buffer,sizeof(buffer)))>0){		//send file
			char *temp_pointer=buffer;
			while(n>0){
				int sent=write(sock_remote_client,temp_pointer,n);
				n-=sent; temp_pointer+=sent;
			}
		}
	}

//-------------------------------------------------------------
	//intialize client module
	void client_init(char *remote_address, char *remote_port){
		timestamp=ctime(&cur);
		fout<<timestamp<<"Client module initialising..."<<endl;
		if((sock_remote_serv=socket(AF_INET,SOCK_STREAM,0))<0)
			handle_error(SOCKET_ERROR);
		if(!strcmp(remote_address,"localhost")){
			struct hostent *h_serv;
			h_serv=gethostbyname(remote_address);
			memcpy(&addr_remote_serv.sin_addr,h_serv->h_addr_list[0],h_serv->h_length);
		}
		else if(inet_pton(AF_INET,remote_address,&addr_remote_serv.sin_addr)<=0)
			handle_error(INV_ADDR);
		int port_remote_serv=stoi(remote_port);
		addr_remote_serv.sin_family=AF_INET;
		addr_remote_serv.sin_port=htons(port_remote_serv);
		if(connect(sock_remote_serv,(struct sockaddr *) &addr_remote_serv,sizeof(addr_remote_serv))<0)
			handle_error(CONNECT_ERROR);
	}

//-------------------------------------------------------------
	//initialize download server	
	void download_server_init(){
		if((sock_down_serv=socket(AF_INET,SOCK_STREAM,0))<0)
			handle_error(SOCKET_ERROR);
		addr_down_serv.sin_family=AF_INET;
		addr_down_serv.sin_addr.s_addr=INADDR_ANY;
		addr_down_serv.sin_port=htons(port_down_serv);
		if(bind(sock_down_serv,(struct sockaddr*) &addr_down_serv,sizeof(addr_down_serv))<0)
			handle_error(BIND_ERROR);		
		listen(sock_down_serv,LQUEUE);
		socklen_t clen=sizeof(addr_remote_client);
		while(true){
			if((sock_remote_client=accept(sock_down_serv,(struct sockaddr *) &addr_remote_client,&clen))<0)
				handle_error(INV_CONN);
			if(!fork()){
				serve_request();		//handle download task
				shut_down();
			}
			else
				close(sock_remote_client);
		}
	}

//-------------------------------------------------------------
	//initialize client subprocesses - download server & client modules
	void init(char* remote_address, char* remote_port, char* client_port){ 
		port_down_serv=stoi(client_port);
		if(!(client_pid=fork())){
			sleep(1);				//start download server first
			menu(remote_address,remote_port);
			shut_down();
		}
		else
			download_server_init();
	}

};

//-------------------------------------------------------------
int main(int argc,char **argv){
	cur=time(0);
	Client c;			//application client object
	signal(SIGCHLD,SIG_IGN);
	if(argc<4)
		c.handle_error(NOT_ENOUGH_ARGS);
	c.init(argv[1],argv[2],argv[3]);
	return 0;
}
