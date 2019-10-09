//-------------------------------------------------------------
// @ Sanjoy Chowdhury - 20172123  Server
//-------------------------------------------------------------

#include<iostream>
#include<fstream>
#include<cstdio>
#include<cstdlib>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<errno.h>
#include<cstring>
#include<signal.h>
#include<string>
#include<vector>
#include<sstream>
#include<arpa/inet.h>
#include<ctime>

#define MAXSIZE 1000000
#define MAXPATH 4096
#define IPSIZE 100
#define LQUEUE 5
#define NOT_ENOUGH_ARGS 1112
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

const string db("repo.txt");
ofstream fout("server_log.txt",ios::out|ios::trunc);
char *timestamp;
time_t cur;

class Server{

private:

	int sock_repo_serv,sock_remote_client;
	struct sockaddr_in addr_repo_serv,addr_remote_client;
	int port_repo_serv;
	fstream finout;

public:

	Server(){
		sock_repo_serv=sock_remote_client=0;
		memset(&addr_repo_serv,0,sizeof(addr_repo_serv));
		memset(&addr_remote_client,0,sizeof(addr_remote_client));
		port_repo_serv=0;
		finout.open(db,ios::in|ios::out|ios::ate);
		if(!finout.is_open()){
			timestamp=ctime(&cur);
			cerr<<"Repository could not be loaded"<<endl;
			fout<<timestamp<<"Repository could not be loaded"<<endl;
			shutdown();
		}
	}

//-------------------------------------------------------------
	//graceful shutdown
	//incase of error
	void shutdown(){
		close(sock_repo_serv);
		close(sock_remote_client);
		exit(0);
	}

//-------------------------------------------------------------
	//error handling function
	void handle_error(int errsv){
		timestamp=ctime(&cur);
		fout<<timestamp<<"Error: ";
		cerr<<"Error: ";
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
		else
			cerr<<"errno number "<<errsv<<endl;
		shutdown();
}

//-------------------------------------------------------------
	//function to serve the request made by the client
	void serve_request(){
		char message[MAXSIZE],*token;
		ssize_t n;
		if((n=read(sock_remote_client,message,MAXSIZE))>0){
			token=strtok(message,"#");
			string file_output;
			if(!strcmp(token,"share*_")){      //share request received
				timestamp=ctime(&cur);
				fout<<timestamp<<"Share request received..."<<endl;
				token=strtok(NULL,"#");
				file_output+=string(token)+':';
				token=strtok(NULL,"#");
				file_output+=string(token)+':';
				char address[INET_ADDRSTRLEN];
				if(inet_ntop(AF_INET,&addr_remote_client.sin_addr.s_addr,address,INET_ADDRSTRLEN)==NULL)
					handle_error(IP_ERROR);
				file_output+=string(address);
				finout<<file_output<<endl;
			}
			else if(!strcmp(token,"search*_")){    //search request received
				timestamp=ctime(&cur);
				fout<<timestamp<<"Search request received..."<<endl;
				token=strtok(NULL,"#");
				string filename(token),tempstr;
				vector<string> output;
				finout.seekg(ios::beg);
				while(finout.good()){
					string line;
					getline(finout,line);
					istringstream temp(line);
					getline(temp,tempstr,':');
					if(tempstr.find(filename)!=std::string::npos){
						output.push_back(line);
						file_output+=line+'#';
					}
				}
				if((n=write(sock_remote_client,file_output.c_str(),file_output.size()+1))<0)   
				 	handle_error(SEND_ERROR);
			}
		}
	}

//-------------------------------------------------------------
	//initialize repository server to accept incoming connections	
	void init(char *server_port){
		timestamp=ctime(&cur);
		fout<<timestamp<<"Server initialising..."<<endl;
		port_repo_serv=stoi(server_port);      
		if((sock_repo_serv=socket(AF_INET,SOCK_STREAM,0))<0)
			handle_error(SOCKET_ERROR);
		addr_repo_serv.sin_family=AF_INET;
		addr_repo_serv.sin_addr.s_addr=INADDR_ANY;
		addr_repo_serv.sin_port=htons(port_repo_serv);
		if(bind(sock_repo_serv,(struct sockaddr*) &addr_repo_serv,sizeof(addr_repo_serv))<0)
			handle_error(BIND_ERROR);
		listen(sock_repo_serv,LQUEUE);
		socklen_t clen=sizeof(addr_remote_client);
		while(true){
			if((sock_remote_client=accept(sock_repo_serv,(struct sockaddr *) &addr_remote_client,&clen))<0)
				handle_error(INV_CONN);
			timestamp=ctime(&cur);
			fout<<timestamp<<"Client connected..."<<endl;
			if(!fork()){
				serve_request();	
				timestamp=ctime(&cur);
				fout<<timestamp<<"Request served..."<<endl;
				shutdown();
			}
			else
				close(sock_remote_client);
		}
	}

};

//-------------------------------------------------------------
int main(int argc,char **argv){
	cur=time(0);
	Server s;		//application server object
	signal(SIGCHLD,SIG_IGN);
	if(argc<2)
		s.handle_error(NOT_ENOUGH_ARGS);
	s.init(argv[1]);
	return 0;
}
