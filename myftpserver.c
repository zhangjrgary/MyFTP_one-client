# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <netinet/in.h>
# include <dirent.h>
# include "myftp.h"

# define MAX_INT 32767
# define OPEN_CONN_REQUEST_TYPE (char)0xa1
# define OPEN_CONN_REPLY_TYPE (char)0xa2
# define AUTH_REQUEST_TYPE (char)0xa3
# define AUTH_REPLY_TYPE (char)0xa4
# define LS_REQUEST_TYPE (char)0xa5
# define LS_REPLY_TYPE (char)0xa6
# define GET_REQUEST_TYPE (char)0xa7
# define GET_REPLY_TYPE (char)0xa8
# define FILE_DATA_TYPE (char)0xFF
# define PUT_REQUEST_TYPE (char)0xa9
# define PUT_REPLY_TYPE (char)0xaa
# define QUIT_REQUEST_TYPE (char)0xab
# define QUIT_REPLY_TYPE (char)0xac
# define FILE_PATH "./filedir/"


static struct message_s decode_msg(char *buff) {
	struct message_s *recv_msg = (struct message_s *)buff;
	struct message_s decoded_msg;
	strcpy(decoded_msg.protocol, recv_msg->protocol);
	decoded_msg.type = recv_msg->type;
	decoded_msg.status = recv_msg->status;
	decoded_msg.length = recv_msg->length;
	
	return decoded_msg;
}


int main(int argc, char** argv){
	
	int PORT;
	PORT = atoi(argv[1]);
	int sd=socket(AF_INET,SOCK_STREAM,0);
	int client_sd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	struct message_s out_msg;
	struct message_s in_msg;
	FILE *fr;
	
	// read in Auth file
	if(access("access.txt", F_OK) != -1 ) {
		printf("AUTH File Detected!\n");
	} else {
		printf("AUTH File not Detected! Exiting...\n");
	}
	fr = fopen("access.txt","rt");
	char line[100];
	int usercount;
	char username[100][100];
	char password[100][100];
	usercount = 0;
	while (fgets(line, MAX_INT, fr) != NULL) {
		char *token;
		token = strtok(line, " ");
		strcpy(username[usercount], token);
		token = strtok(NULL, " ");
		strcpy(password[usercount], token);
		usercount ++;
		if (usercount > 99) {
			usercount = 99;
		}
	}
	
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons(PORT);
	if(bind(sd,(struct sockaddr *) &server_addr,sizeof(server_addr))<0){
		printf("bind error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	if(listen(sd,3)<0){
		printf("listen error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	int addr_len=sizeof(client_addr);
	if((client_sd=accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
		printf("accept error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	} 
	
	
	
	while(1){
		
		char buff[100];
		char payload[1000];
		memset(payload, 0, sizeof(payload));
		strcpy(buff, "");
		int len;
		if((len=read(client_sd,buff,sizeof(struct message_s)))<0){
			printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
			exit(0);
		}
		
		if (strcmp(buff, "") != 0) {
			in_msg = decode_msg(buff);
		}
		
		// validation of the information
		if (strstr(in_msg.protocol, "myftp") == NULL) {
			printf("Invalid Protocol!\n");
			continue;
		}
		
		// OPEN_CONN_REQUEST
		if (in_msg.type == OPEN_CONN_REQUEST_TYPE) {
			printf("OPEN_CONN_REQUEST Received!\n");
			memset (&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = OPEN_CONN_REPLY_TYPE;
			out_msg.status = '1';
			out_msg.length = 12;		
			if((len=write(client_sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				continue;
			} else {
				printf("OPEN_CONN_REPLY Sent!\n"); 
			}
		}
		
		// AUTH_REQUEST
		if (in_msg.type == AUTH_REQUEST_TYPE) {
			char name[100];
			char pass[100];
			int isSuccess = 0;
			printf("AUTH_REQUEST Received!\n");
			read(client_sd, payload, in_msg.length-12);
			char *token;
			token = strtok(payload, " ");
			strcpy(name, token);
			token = strtok(NULL, " ");
			strcpy(pass, token);
			for (int i=0; i<=usercount; i++) {
				if (strcmp(name, username[i]) == 0 && strcmp(pass, password[i]) == 0) {
					isSuccess = 1;
				}
			}
			memset(&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = AUTH_REPLY_TYPE;
			if (isSuccess == 0) {
				out_msg.status = '0';
			} else {
				out_msg.status = '1';
			}
			out_msg.length = 12;		
			if((len=write(client_sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				continue;
			} else {
				printf("AUTH_REPLY Sent!\n"); 
			}
			
			
			if (isSuccess == 0) {
				if((client_sd=accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
				printf("accept error: %s (Errno:%d)\n",strerror(errno),errno);
				exit(0);
			} 
			}
		}
		
		
		// LS_REQUEST
		if (in_msg.type == LS_REQUEST_TYPE) {
			DIR *dir;
			dir = opendir(FILE_PATH);
			if (!dir) {
				printf("ERROR: Cannot Open Folder!\n");
				continue;
			}
			for (;;) {
				struct dirent entry;
				struct dirent *result;
				char *name;
				char sub_path[256];
		 
				int error = readdir_r(dir, &entry, &result);
				if (error != 0) {
					printf("ERROR: Cannot Read Folder!\n");
					break;
				}
		 
				// readdir_r returns NULL in *result if the end 
				// of the directory stream is reached
				if (result == NULL)
					break;
		 
				name = result->d_name;
				if ((strcmp(name, ".") == 0) || (strcmp(name, "..") == 0))
					continue;
				
				strcat(payload, " ");
				strcat(payload, name);
			}
			closedir(dir);
			
			memset(&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = LS_REPLY_TYPE;
			out_msg.status = '0';
			out_msg.length = 12 + strlen(payload);
			if((len=write(client_sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				continue;
			} else {
				printf("LS_REPLY Sent!\n"); 
			}
			
			write(client_sd, payload, strlen(payload));
		}
		
		// GET_REQUEST
		if (in_msg.type == GET_REQUEST_TYPE) {
			char fname[100];
			char tmp[100];
			int isFound = 0;
			printf("GET_REQUEST Received!\n");
			read(client_sd, payload, in_msg.length-12);
			strncpy(tmp, payload, strlen(payload)-1);
			strcpy(fname, FILE_PATH);
			strcat(fname, tmp);
			if(access(fname, F_OK) != -1 ) {
				isFound = 1;
				printf("FILE EXIST\n");
			} else {
				printf("FILE NOT EXIST\n");
			}
			memset(&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = AUTH_REPLY_TYPE;
			if (isFound == 0) {
				out_msg.status = '0';
			} else {
				out_msg.status = '1';
			}
			out_msg.length = 12;		
			if((len=write(client_sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				continue;
			} else {
				printf("GET_REPLY Sent!\n"); 
			}
			
			if (isFound == 1) {
				FILE * fp;
				char * line = NULL;
				size_t len = 0;
				ssize_t read;
				long size = 0;
				fp = fopen(fname, "r");
				while ((read = getline(&line, &len, fp)) != -1) {
					size = size + strlen(line);
				}
				
				memset(&out_msg, 0, sizeof(out_msg));
				strcpy(out_msg.protocol, " myftp");
				out_msg.protocol[0] = 0xe3;
				out_msg.type = FILE_DATA_TYPE;
				out_msg.status = '0';
				out_msg.length = 12 + size;
				if((len=write(client_sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
					printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
					continue;
				} else {
					printf("FILE_DATA Sent!\n"); 
				}
				
				fp = fopen(fname, "r");
				line = NULL;
				len = 0;
				while ((read = getline(&line, &len, fp)) != -1) {
					write(client_sd, line, strlen(line));
				}
				fclose(fp);

			}
		}
		
		// PUT_REQUEST
		if (in_msg.type == PUT_REQUEST_TYPE) {	
			char fname[100];
			char tmp[100];
			read(client_sd, tmp, in_msg.length-12);
			strcpy(fname, FILE_PATH);
			strcat(fname, tmp);
			memset(&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = PUT_REPLY_TYPE;
			out_msg.status = '0';
			out_msg.length = 12;
			if((len=write(client_sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				continue;
			} else {
				printf("PUT_REPLY Sent!\n"); 
			}
			
			// receive file data
			if((len=read(client_sd,buff,sizeof(struct message_s)))<0){
				printf("Receive error: %s (Errno:%d)\n", strerror(errno),errno);
				exit(0);
			}
			if (strcmp(buff, "") != 0) {
				in_msg = decode_msg(buff);
			}
			
			// validation of the msg
			if (strstr(in_msg.protocol, "myftp") == NULL) {
				printf("Invalid Protocol!\n");
				continue;
			}
			
			int size;
			int scount = 0;
			int readsize = 500;
			char fstr[1000];
			size = in_msg.length - 12;
			FILE *fout;
			fout = fopen(fname, "w");
			
			while (scount < size) {
				memset(fstr, 0, sizeof(fstr));
				read(client_sd, fstr, readsize);
				scount = scount + strlen(fstr);
				fprintf(fout, "%s", fstr);
			}
			printf("File downloaded.\n");
			fclose(fout);
	
		}
		
		if (in_msg.type == QUIT_REQUEST_TYPE) {	
			memset(&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = QUIT_REPLY_TYPE;
			out_msg.status = '0';
			out_msg.length = 12;
			if((len=write(client_sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				continue;
			} else {
				printf("QUIT_REPLY Sent!\n"); 
			}
			
			close(client_sd);
			exit(0);
			
		}
		
		printf("----------------------\n");
		
	}
	close(sd);
	return 0;
}
