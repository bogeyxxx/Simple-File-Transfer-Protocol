/* 		Test stream I/O (client part)
 */

#include  <stdlib.h>
#include  <stdio.h>
#include  <sys/types.h>        
#include  <sys/socket.h>
#include  <netinet/in.h>       /* struct sockaddr_in, htons, htonl */
#include  <netdb.h>            /* struct hostent, gethostbyname() */ 
#include  <string.h>
#include  <sys/types.h> 
#include  <unistd.h>
#include  <dirent.h>
#include  <fcntl.h>
     

#define   SERV_TCP_PORT  40260 /* port number */
#define   BUFSIZE        1024

void ldir(char* path)
{
DIR *dp;
struct dirent *file;
char fileName[BUFSIZE];

if((dp = opendir(path)) == NULL)/* open directory */
{
	printf("Error in opening directory %s\n", path);
	exit(1);
}
if((file = readdir(dp)) < 0)
{
	perror("lstat error");
	exit(1);
}
while((file =readdir(dp)) != NULL)//read all files inside the directory
{
	strncpy(fileName, file->d_name, BUFSIZE);//get file name
	fileName[BUFSIZE] = '\0';
	printf("%s ", fileName);
}
printf("\n");
closedir(dp);
}

int main(int argc, char *argv[])
{
int sd, nr, nw, i=0, len, ser_addr_len;
char buf[BUFSIZE], buffer[BUFSIZE], host[60], *str;
struct sockaddr_in ser_addr; struct hostent *hp;

/* get server host name */
if (argc==1)  /* assume server running on the local host */
	gethostname(host, sizeof(host));
else if (argc == 2) /* use the given host name */ 
	strcpy(host, argv[1]);
else
{
	printf("Usage: %s [<server_host_name>]\n", argv[0]);
	 exit(1); 
}

/* get host address, & build a server socket address */
bzero((char *) &ser_addr, sizeof(ser_addr));
ser_addr.sin_family = AF_INET;
ser_addr.sin_port = htons(SERV_TCP_PORT);
if ((hp = gethostbyname(host)) == NULL)
{
	printf("host %s not found\n", host);
	exit(1);   
}
ser_addr.sin_addr.s_addr = * (u_long *) hp->h_addr;

/* create TCP socket & connect socket to server address */
sd = socket(PF_INET, SOCK_STREAM, 0);
ser_addr_len = sizeof(ser_addr);
if (connect(sd, (struct sockaddr *) &ser_addr, ser_addr_len)<0)
{ 
	perror("client connect");
	exit(1);
}

while (++i)
{
	
	bzero(buf, BUFSIZE);
	bzero(buffer, BUFSIZE);

	printf("client[%d]: > ", i);	/* client input */
	fgets(buf, BUFSIZE, stdin);
	nr = strlen(buf);
	if (buf[nr-1] == '\n')
	{
		buf[nr-1] = '\0';
		--nr;
	}
	if (nr <=  0)
		continue;

	str = strchr(buf,' ');	/* check if include space */
	if (strcmp(buf, "quit")==0)	/* quit */
	{
		printf("Bye from client\n");
		exit(0);
	}
	else if (strcmp(buf, "lpwd")==0)	/* lpwd */
	{
		
		if(getcwd(buffer, sizeof(buffer)) != NULL) 
			printf("Current directory : %s\n",buffer);
		continue;
		
	}
	else if (strcmp(buf, "ldir")==0)	/* ldir*/
	{
		
		if(getcwd(buffer, sizeof(buffer)) != NULL) 
			ldir(buffer);
		continue;
	}
	else if(str!=NULL)	/* with path or file name */
	{
		char * function, *file, *ptr;
		char parentFile[BUFSIZE];
		

		function = strtok(buf," ");
		file = strtok(NULL," ");
		if(strcmp(function,"lcd")==0)	/* function lcd path */
		{

			if (strcmp(file,".")==0)	/* path . */
			{
				
				if(getcwd(buffer, sizeof(buffer)) != NULL) 
					printf("Current directory : %s\n",buffer);
				continue;
			}
			else if (strcmp(file,"..")==0)	/* path .. */
			{
				
				if(getcwd(buffer, sizeof(buffer)) != NULL) 
				{
					ptr = strrchr(buffer,'/');
					strncpy(parentFile, buffer, ptr-buffer);
					parentFile[ptr-buffer]='\0';
	
					if((chdir(parentFile)) ==0)
					{
						printf("Current directory : %s\n",parentFile);
					}	
					else
					{
						printf("Error in change directory\n");
					}
				}
				continue;
					
			}
			else
			{
				if((chdir(file)) ==0)	/* path */
				{
					printf("Current directory : %s\n",file);
				}
				else
				{
					printf("Error in change directory\n");
				}
				continue;
			}
		}
		else if(strcmp(function,"cd")==0)	/* function cd */
		{

			strcat(function, file);
			strcpy(buf,function);
		}
		else if(strcmp(function,"get")==0)	/* function get */
		{
			int f2;
	
			f2 = open(file,O_RDWR|O_CREAT,0777);
			if((f2 < 0))
			{ 
				//printf("Open Error!\n"); 
        			continue; 
			}
			strcat(function, file);
			strcpy(buf,function);
			nw = send(sd, buf, sizeof(buf), 0);	/* send request to server */
			if (nw <= 0)
				exit(1);    /* send error */
			printf("client[%d]: send %s\n", i, buf);

			bzero(buf, BUFSIZE);
			while((nr = recv(sd, buf, sizeof(buf), 0)) > 0)	/* read from server */
			{
				buf[nr] = '\0';
				fprintf(stdout,"%s\n",buf);
				len=strlen(buf);

				if(nw = write(f2, buf, len)<0)	/* write into file */
				{
					exit(1);
				}
				
				printf("client[%d]: receive %s\n", i,buf);
				break;
				
			}
			close(f2);
			printf("client[%d]: file closed\n", i);
			continue;

		}
		else if(strcmp(function,"put")==0)	/* function put */
		{
			
			int f1;
			char line[BUFSIZE];

			
			strcat(function, file);
			strcpy(buf,function);
			nw = send(sd, buf, sizeof(buf), 0);	/* send request to server */

			bzero(buf, BUFSIZE);
			nr = read(sd, buf, sizeof(buf));	/* get response from server */
			
			file = buf;
			
			printf("file %s\n", file);
			f1 = open(file,O_RDWR);
			if((f1 < 0))
			{ 
				//printf("Open Error!\n"); 
        			continue; 
			}
				bzero(buf, BUFSIZE);
				while((nr = read(f1, line, sizeof(line))) > 0)	/* read from file */
				{
						
					printf("nr %d\n", nr);
					line[nr] = '\0';
					fprintf(stdout,"%s\n",line);
					len=strlen(line);
					
					strncpy(buf, line, len);
					nw = send(sd, buf, len, 0);	/* send to server */
					if (nw <= 0)
						exit(1);    /* send error */
					printf("client[%d]: send %s\n", i,buf);
				}
		
			close(f1);
			printf("file closed\n");
			continue;
		}
	}
	else if (strcmp(buf, "lcd")==0)	/* function lcd */
	{
		char *env;
		env = getenv("HOME");
		chdir(env);
		
		if(getcwd(buffer, sizeof(buffer)) != NULL) 
			printf("%s\n",buffer);	
		continue;
	}
          
/* send the message to server */
	nw = sendto(sd, buf, sizeof(buf), 0, (struct sockaddr *) &ser_addr, ser_addr_len);
	if (nw <= 0)
		exit(1);   /* send error */
	printf("client[%d]: send %s\n", i, buf);

/* receive the message from server */
	bzero(buf, BUFSIZE);
	nr = recvfrom(sd, buf, BUFSIZE, 0, (struct sockaddr *) &ser_addr, &ser_addr_len);
	if (buf[nr-1] == '\n')
	{
		buf[nr-1] = '\0';
		--nr;
	}
	if (nr <= 0)
		exit(1);   /* receive error */
	printf("client[%d]: receive %s\n", i, buf);
}
}


