# Socket_Programming_C
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/wait.h>

#define BUF_SIZE 1024

typedef struct frame{
	int frame_no;
	char Data[BUF_SIZE];
}FRAME;

void clearbuffer(char* buffer){
	for(int i=0;i<BUF_SIZE;i++){
		buffer[i] = 0;
	}
}

void clearframe(FRAME* frame){
	for(int i=0;i<BUF_SIZE;i++){
		frame->Data[i] = 0;
	}
}

void server_process(char*);
void client_process(char*,char*);

int main(int argc , char** argv){
	if(argc < 4){
		printf("[ERROR] : ARGUMENT MISSING");
		exit(-1);
	}
	pid_t pid = fork();
	if(pid<0){
		perror("[ERROR]");
		exit(-1);
	}else
	if(pid==0){ // inside child
		client_process(argv[1],argv[2]);
		wait(NULL);
	}else
	if(pid>0){ //Parent Process
		server_process(argv[3]);
		wait(NULL);
	}
	return 0;
}

void server_process(char* port_number){
	int socketfd;
	char buffer[BUF_SIZE];
	socketfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(socketfd == -1){
		perror("[ERROR]");
		exit(-1);
	}

	struct sockaddr_in server_address,client_info;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(port_number));
	server_address.sin_addr.s_addr = INADDR_ANY;

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	int option_set = setsockopt(socketfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
	if(option_set == -1){
		perror("[ERROR]");
		exit(-1);
	}
	int bind_status = bind(socketfd,(struct sockaddr*)&server_address,sizeof(server_address));
	if(bind_status == -1){
		perror("[ERROR]");
		exit(-1);
	}
	FILE* fp;
	int sizeof_client_info = sizeof(client_info);
	char file_name[100];
	char path_name[100];
	char list_address[100];
	while(1){
		int status = recvfrom(socketfd,list_address,sizeof(list_address),0,(struct sockaddr*)&client_info,&sizeof_client_info);
		if(status > 0 ){
			break;
		}
	}
	sleep(2);
	FILE * listptr = fopen(list_address,"r");
	if(listptr==NULL){
		strcpy(buffer,"[ERROR] : UNABLE TO OPEN LIST");
		printf("%s\n",buffer);
		sendto(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&client_info,sizeof(client_info));
		exit(-1);
	}
	strcpy(buffer,"[LIST FOUND]");
	printf("%s\n",buffer);
	sendto(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&client_info,sizeof(client_info));
	char real_path[100] = "./Download/";
	while(1){
		char temp_copy[100];
		fgets(temp_copy,80,listptr);
		if(feof(listptr)){
			strcpy(buffer,"@END@");
			sendto(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&client_info,sizeof(client_info));
			break;
		}
		memcpy(file_name,temp_copy,strlen(temp_copy)-1);
		file_name[strlen(file_name)] = 0;
		printf("FILE NAME : %s]\n",file_name);
		strcpy(path_name,real_path);
		strcat(path_name,file_name);
		FILE* fp;
		fp = fopen(path_name,"rb");
		if(fp == NULL){
			strcpy(buffer,"[ERROR] : FILE NOT FOUND");
			printf("%s : %s\n",buffer,path_name);
			sendto(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&client_info,sizeof(client_info));
			continue;
		}

		strcpy(buffer,file_name);
		sendto(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&client_info,sizeof(client_info));
		memset(buffer,0,BUF_SIZE);	
		while(1) {
			int status = recvfrom(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&client_info,&sizeof_client_info);
			if(status>0) break;	
		}
		if(strcmp(buffer,"[NOTICE] : FILE ALREADY PRESENT")==0){
			long int current_position[1]={0};
			
			while(1) {
				int status = recvfrom(socketfd,current_position,sizeof(current_position),0,(struct sockaddr*)&client_info,&sizeof_client_info);
				if(status > 0) break;	
			}
			fseek(fp,current_position[0],SEEK_SET);
			printf("[CURRENT POSITION] : %ld\n",current_position[0]);
		}
		
		FRAME frame;
		frame.frame_no = 0;
		int Ack[1] = {0};
		int flag=0;
		while(1){
			clearbuffer(buffer);
			clearframe(&frame);
			int nbytes = fread(buffer,1,BUF_SIZE,fp);
			memcpy(&(frame.Data),buffer,nbytes);
			while(1){
				if(nbytes>0){
					int Data_sent = sendto(socketfd,&frame,nbytes+sizeof(int),0,(struct sockaddr*)&client_info,sizeof(client_info));
					recvfrom(socketfd,Ack,sizeof(Ack),0,(struct sockaddr*)&client_info,&sizeof_client_info);
				}
				if((Ack[0]==1 && frame.frame_no==0)||(Ack[0]==0 && frame.frame_no==1)){
					frame.frame_no = Ack[0] == 0 ? 0 : 1;
					break;
				}
				if(nbytes<BUF_SIZE){
					if(feof(fp)){
						flag=1;
						printf("[FILE SENT]\n");
						break;
					}
					if(ferror(fp)){
						printf("[ERROR READING]\n");
					}
				}		
			}
			if(flag==1){
				break;
			}
		}
		clearframe(&frame);
		frame.frame_no = -1;
		strcpy(frame.Data,"DONE");
		sendto(socketfd,&frame,sizeof(frame),0,(struct sockaddr*)&client_info,sizeof(client_info));
		fclose(fp);
	}
	fclose(listptr);	
	close(socketfd);
}
void client_process(char* ipV4,char* port_number){
	int socketfd;
	char buffer[BUF_SIZE];
	socketfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(socketfd == -1){
		perror("[ERROR]");
		exit(-1);
	}
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(port_number));;
	int status = inet_aton(ipV4,&server_address.sin_addr);
	if(status==-1){
		perror("[ERROR]");
		exit(-1);
	}
	int sizeof_server_address = sizeof(server_address);
	char list_address[100];
	char list_name[100];
	char file_name[100];
	printf("[ENTER LIST ADDRESS] : ");
	scanf(" %[^\n]",list_address);
	printf("[ENTER LIST NAME] : ");
	scanf(" %[^\n]",list_name);
	strcat(list_address,list_name);
	sendto(socketfd,list_address,sizeof(list_address),0,(struct sockaddr*)&server_address,sizeof(server_address));
	recvfrom(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&server_address,&sizeof_server_address);
	if(strcmp(buffer,"[ERROR] : UNABLE TO OPEN LIST")==0){
		printf("%s\n",buffer);
		exit(-1);
	}
	char location[100] = "./Download/";
	char file[100]= "./Download/";
	while(1){
		clearbuffer(buffer);
		recvfrom(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&server_address,&sizeof_server_address);
		if(strcmp(buffer,"@END@")==0){
			break;
		}
		if(strcmp(buffer,"[ERROR] : FILE NOT FOUND")==0){
			continue;
		}
		strcpy(file,location);	
		strcat(file,buffer);
		FILE* fp=fopen(file,"r");
		long int current_position[1]={0};
		if(fp!=NULL){
			rewind(fp);
			memset(buffer,0,BUF_SIZE);
			strcpy(buffer,"[NOTICE] : FILE ALREADY PRESENT");
			sendto(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&server_address,sizeof(server_address));
			fseek(fp,0,SEEK_END);
			current_position[0] = ftell(fp);
			sendto(socketfd,current_position,sizeof(current_position),0,(struct sockaddr*)&server_address,sizeof(server_address));
		}else{
			strcpy(buffer,"[NOTICE] : FILE NOT PRESENT");
			sendto(socketfd,buffer,sizeof(buffer),0,(struct sockaddr*)&server_address,sizeof(server_address));
		}
		fp = fopen(file,"ab");
		FRAME frame;
		int Ack[1]={1};
		int old_frame_no = 1;
		while(1){
			clearbuffer(buffer);
			clearframe(&frame);
			int recvbytes = recvfrom(socketfd,&frame,sizeof(frame),0,(struct sockaddr*)&server_address,&sizeof_server_address);
			memcpy(buffer,&(frame.Data),recvbytes-4);
			if(recvbytes < 0){
				perror("[ERROR]");
				break;
			}
			if(recvbytes<0){
				perror("[ERROR]");
			}if((strcmp("DONE",frame.Data)==0) && (frame.frame_no == -1)){
		//		printf("[RECEIVED]\n");
				break;
			}
			if( old_frame_no != frame.frame_no ){
				int write_status = fwrite(buffer,1,recvbytes-sizeof(int),fp);
				old_frame_no = frame.frame_no;
			}
			Ack[0] = frame.frame_no == 1 ? 0 : 1;
			sendto(socketfd,Ack,sizeof(Ack),0,(struct sockaddr*)&server_address,sizeof(server_address));
			recvbytes = 0;
		}
		fclose(fp);
	}
	close(socketfd);
}
