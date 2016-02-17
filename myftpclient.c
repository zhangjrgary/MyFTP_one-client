# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include "myftp.h"

# define PORT 12345
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
	
	int isConn = 0;
	int isAuth = 0;
	int sd;
	char str[MAX_INT];
	char input[5][MAX_INT];
	int count;
	int len;
	struct message_s out_msg;
	struct message_s in_msg;
	while (1) {
		char buff[100];
		char payload[1000];
		memset(str,0,strlen(str));
		memset(input,0,sizeof(input));
		memset(payload, 0, sizeof(payload));
		// take in input from user
		for (int i=0; i<5; i++) {
			strcpy(input[i], " ");
		}
		fgets(str, MAX_INT, stdin);
		
		if (strstr(str, " ") == NULL) {
			strncpy(input[0], str, strlen(str)-1);
		} else {
			char *token;
			token = strtok(str, " ");
			count = 0;
			while (token != NULL) {
				strcpy(input[count], token);
				count ++;
				token = strtok(NULL, " ");
			}
		}
		// process the input
		// open command
		if (strcmp(input[0], "open") == 0) {
			if (isConn == 1) {
				printf("%s", "Already connected to server!\n");
			} else {
				if ((strcmp(input[1], "") == 0) || (strcmp(input[2],"") == 0)){
					printf("%s", "Insufficient arguments!\n");
					continue;
				}
				sd=socket(AF_INET,SOCK_STREAM,0);
				struct sockaddr_in server_addr;
				memset(&server_addr,0,sizeof(server_addr));
				server_addr.sin_family=AF_INET;
				server_addr.sin_addr.s_addr=inet_addr(input[1]);
				server_addr.sin_port=htons(atoi(input[2]));
				//server_addr.sin_port=htons(12345);
				if(connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
					printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
					exit(0);
				}
				// connection success
				
				// build OPEN_CONN_REQUEST
				
				memset (&out_msg, 0, sizeof(out_msg));
				strcpy(out_msg.protocol, " myftp");
				out_msg.protocol[0] = 0xe3;
				out_msg.type = OPEN_CONN_REQUEST_TYPE;
				out_msg.status = '0';
				out_msg.length = 12;		
				if((len=write(sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
					printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
					exit(0);
				}
				
				// receive response
				if((len=read(sd,buff,sizeof(struct message_s)))<0){
					printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
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
				if (in_msg.type == OPEN_CONN_REPLY_TYPE && in_msg.status == '1') {
					isConn = 1;
					printf("Connection accepted.\n");
				}
				
			}
				
		}
		
		// auth command
		if (strcmp(input[0], "auth") == 0) {
			if (isConn == 0) {
				printf("ERROR: Authentication Failed. No connection detected!\n");
				continue;
			}
			memset (&out_msg, 0, sizeof(out_msg));
			strcpy(payload, input[1]);
			strcat(payload, " ");
			strcat(payload, input[2]);
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = AUTH_REQUEST_TYPE;
			out_msg.status = '0';
			out_msg.length = 12 + strlen(payload);		
			if((len=write(sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				exit(0);
			}
			write(sd, payload, strlen(payload));
			
			// receive reply
			if((len=read(sd,buff,sizeof(struct message_s)))<0){
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
			if (in_msg.type == AUTH_REPLY_TYPE && in_msg.status == '1') {
				isAuth = 1;
				printf("Authentication granted.\n");
			} else {
				printf("ERROR: Authentication rejected. Connection closed.\n");
				isConn = 0;
				isAuth = 0;
			}
		}
		
		// ls command
		if (strcmp(input[0], "ls") == 0) {
			if (isAuth == 0) {
				printf("ERROR: authentication not started. Please issue an authentication command.\n");
				continue;
			}
			memset (&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = LS_REQUEST_TYPE;
			out_msg.status = '0';
			out_msg.length = 12;		
			if((len=write(sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				exit(0);
			}
			
			// receive reply
			if((len=read(sd,buff,sizeof(struct message_s)))<0){
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
			
			read(sd, payload, in_msg.length-12);
			printf("----- file list start -----\n");
			char *token;
			token = strtok(payload, " ");
			while (token != NULL) {
				printf("%s\n", token);
				token = strtok(NULL, " ");
			}
			printf("----- file list end -----\n");
		}
		
		// get command 
		if (strcmp(input[0], "get") == 0) {
			if (isAuth == 0) {
				printf("ERROR: authentication not started. Please issue an authentication command.\n");
				continue;
			}
			memset (&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = GET_REQUEST_TYPE;
			out_msg.status = '0';
			strcpy(payload, input[1]);
			out_msg.length = 12 + strlen(payload);
			if((len=write(sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				exit(0);
			}
			write(sd, payload, strlen(payload));
			
			// receive reply
			if((len=read(sd,buff,sizeof(struct message_s)))<0){
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
			
			if (in_msg.status == '0') {
				printf("ERROR: File not exists!\n");
			}
			if (in_msg.status == '1') {
				if((len=read(sd,buff,sizeof(struct message_s)))<0){
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
				char fname[100];
				size = in_msg.length - 12;
				FILE *fout;
				strncpy(fname, input[1], strlen(input[1])-1);
				fout = fopen(fname, "w");
				
				while (scount < size) {
					memset(fstr, 0, sizeof(fstr));
					read(sd, fstr, readsize);
					scount = scount + strlen(fstr);
				}
				printf("File downloaded.\n");
				fclose(fout);
					
			}
		}
		
		// put command
		if (strcmp(input[0], "put") ==0) {
			char fname[100];
			strncpy(fname, input[1], strlen(input[1])-1);
			if(access(fname, F_OK) == -1 ) {
				printf("ERROR: File Not Exists!");
				continue;
			}
			FILE * fp;
			char * line = NULL;
			size_t lgth = 0;
			ssize_t read1;
			long size = 0;
			fp = fopen(fname, "r");
			while ((read1 = getline(&line, &lgth, fp)) != -1) {
				size = size + strlen(line);
			} 
			memset(&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = PUT_REQUEST_TYPE;
			out_msg.status = '0';
			out_msg.length = 12 + strlen(fname);
			if((len=write(sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				continue;
			} else {
				printf("PUT_REQUEST Sent!\n"); 
			}
			write(sd, fname, strlen(fname));
			
			// receive reply
			if((len=read(sd,buff,sizeof(struct message_s)))<0){
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
			if (in_msg.type == PUT_REPLY_TYPE) {
			
				memset(&out_msg, 0, sizeof(out_msg));
				strcpy(out_msg.protocol, " myftp");
				out_msg.protocol[0] = 0xe3;
				out_msg.type = FILE_DATA_TYPE;
				out_msg.status = '0';
				out_msg.length = 12 + size;
				if((len=write(sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
					printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
					continue;
				}
				
				fp = fopen(fname, "r");
				line = NULL;
				lgth = 0;
				while ((read1 = getline(&line, &lgth, fp)) != -1) {
					write(sd, line, strlen(line));
				}
				
				printf("%s\n", "File Uploaded.");
				fclose(fp);
			}
		
		}
		
		// quit command
		if (strcmp(input[0], "quit") ==0) {
			memset(&out_msg, 0, sizeof(out_msg));
			strcpy(out_msg.protocol, " myftp");
			out_msg.protocol[0] = 0xe3;
			out_msg.type = QUIT_REQUEST_TYPE;
			out_msg.status = '0';
			out_msg.length = 12;
			if((len=write(sd,&out_msg,sizeof(out_msg))) != sizeof(out_msg)){
				printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
				continue;
			}		
			
			// receive reply
			if((len=read(sd,buff,sizeof(struct message_s)))<0){
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
			if (in_msg.type == QUIT_REPLY_TYPE) {
				close(sd);
				printf("Thank you.\n");
				exit(0);
			}
		}
			
	}
	
	
	
	return 0;
	
}
