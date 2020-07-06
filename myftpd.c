/*
 *		Test stream I/O  (server part)
 */
#include  <stdlib.h>
#include  <stdio.h>
#include  <fcntl.h>
#include  <dirent.h>
#include  <unistd.h>
#include  <signal.h>     /* SIGCHLD, sigaction() */
#include  <string.h>     /* strlen(), strcmp() etc */
#include  <errno.h>      /* extern int errno, EINTR, perror() */
#include  <sys/types.h>  /* pid_t, u_long, u_short */
#include  <sys/wait.h>  /* waitpid */
#include  <sys/stat.h>  /* umask */
#include  <sys/socket.h> /* struct sockaddr, socket(), etc */
#include  <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
                         /* and INADDR_ANY */

#define SERV_TCP_PORT 40260         /* server port no */
#define BUFSIZE 2048

void claim_children()
{
	pid_t pid=1;
	while (pid>0) /* claim as many zombies as we can */
	{ 
		pid = waitpid(0, (int *)0, WNOHANG); 
	} 
}

void daemon_init(void)
{       
	pid_t   pid;
	struct sigaction act;
	if ( (pid = fork()) < 0)
	{
		perror("fork");
		exit(1); 
	}
	else if (pid > 0)
	{
		printf("Server PID: %d\n", pid);
		exit(0);	/* parent goes bye-bye */
	}

	/* child continues */
	setsid();	/* become session leader */
	chdir("/");	/* change working directory */
	umask(0);	/* clear file mode creation mask */

	/* catch SIGCHLD to remove zombies from system */
	act.sa_handler = claim_children;	/* use reliable signal */
	sigemptyset(&act.sa_mask);	/* not to block other signals */
	act.sa_flags   = SA_NOCLDSTOP;	/* not catch stopped children */
	sigaction(SIGCHLD,(struct sigaction *)&act,(struct sigaction *)0);
}

void serve_a_client(int sd, struct sockaddr_in cli_addr, int cli_addr_len)
{
	int nr, nw, len;
	int i =0;
	char buf[BUFSIZE], buffer[BUFSIZE]; 
	char *temp;


	while (++i)
	{
	/* read data from client */
		bzero(buf, BUFSIZE);
		bzero(buffer, BUFSIZE);
		nr = recvfrom(sd, buf, BUFSIZE, 0, (struct sockaddr *) &cli_addr, &cli_addr_len); 
		if (buf[nr-1] == '\n')
		{
			buf[nr-1] = '\0';
			--nr;
		}
		if (nr <= 0)
			exit(1);   /* receive error */
		printf("server[%d]: receive %s\n", i, buf);

		if (strcmp(buf, "pwd")==0)	/* pwd */
		{
			
			if(getcwd(buffer, sizeof(buffer)) != NULL) 
			{
				
				strcpy(buf,buffer);
			}
		}
		else if (strcmp(buf, "dir")==0)	/* dir */
		{
			
			if(getcwd(buffer, sizeof(buffer)) != NULL) 		
			{
				DIR *dp;
				struct dirent *file;
				char fileName[BUFSIZE], temp[BUFSIZE];
				int n = 0;
				if((dp = opendir(buffer)) == NULL)/* open directory */
				{
					printf("Error in opening directory %s\n", buffer);
					exit(1);
				}
				if((file = readdir(dp)) < 0)
				{
					perror("lstat error");
					exit(1);
				}
				while((file =readdir(dp)) != NULL)//read all files inside the directory
				{
					bzero(fileName, BUFSIZE);
					strcpy(fileName, file->d_name);//get file name
					len = strlen(fileName);
					if( n < 1)
					{	
						bzero(temp, BUFSIZE);
						strcpy(temp,"");
						strncpy(temp,fileName, len);
						n++;
					}
					else
					{
						strcat(temp," ");
						strncat(temp,fileName, len);
					} 
				}
				closedir(dp);

				strcpy(buf, temp);

			}
			printf("%s\n", buf);	
				
		}
		else if((nr > 2)&&((strncmp(buf,"put", 3)==0)||(strncmp(buf,"get", 3)==0)||(strncmp(buf,"cd/", 3)==0)||(strncmp(buf,"cd.", 3)==0)))	/* with path or filename */
		{
			char *file, *ptr;
			char parentFile[BUFSIZE];
		
			if(strncmp(buf,"cd", 2)==0)	/* function cd path */
			{
				strtok(buf,"d");
				file = strtok(NULL,"d");
				printf("file %s\n", file);
				
				if (strcmp(file,".")==0)	/* path . */
				{
					
					if(getcwd(buffer, sizeof(buffer)) != NULL) 
					{

						strcpy(buf,buffer);
					}
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
						
								strcpy(buf,parentFile);
							}	
							else
							{
								printf("Error in change directory\n");
							}
					}		
					
				}
				else
				{
					if((chdir(file)) ==0)	/* path */
					{
						strcpy(buf,file);
					}
					else
					{
						printf("Error in change directory\n");
					}
				}
			}
			else if(strncmp(buf,"get", 3)==0)	/* function get */
			{
				file = strtok(buf,"get");
				printf("file %s\n", file);
				int f1;
				char line[BUFSIZE];

				f1 = open(file,O_RDWR|O_CREAT,0777);
				if((f1 < 0))
				{ 
				//	printf("Open Error!\n"); 
						continue; 
				}
				
				bzero(buf, BUFSIZE);
				while((nr = read(f1, line, sizeof(line))) > 0)	/* read from file */
				{ 
					line[nr] = '\0';
					fprintf(stdout,"%s\n",line);
					len=strlen(line);
					
					strncpy(buf, line, len);
					nw = send(sd, buf, len, 0);	/* send to client */
					if (nw <= 0)
						exit(1);    /* send error */
					printf("server[%d]: send %s\n", i,buf);	
					
				}
				close(f1);
				printf("server[%d]: file closed\n", i);
				continue;
			}
			else if(strncmp(buf,"put", 3)==0)	/* function put */
			{
				int f2;
				file = strtok(buf,"put");
				printf("file %s\n", file);

				
				nw = send(sd, file, sizeof(file), 0);	/* send responese to client */
				if (nw <= 0)
					exit(1);    /* send error */
				printf("server[%d]: send %s\n", i,file);	
				f2 = open(file,O_RDWR|O_CREAT,0777);
				if((f2 < 0))
				{ 
					//printf("Open Error!\n"); 
						continue; 
				}

				bzero(buf, BUFSIZE);
				while((nr = recv(sd, buf, sizeof(buf), 0)) > 0)	/* read from client */
				{
				
					buf[nr] = '\0';
					fprintf(stdout,"%s\n",buf);
					len=strlen(buf);

					if(nw = write(f2, buf, len)<=0)	/* write into file */
					{
						exit(1);
					}
					printf("server[%d]: receive %s\n", i,buf);
					break;
					
				}
				close(f2);
				continue;
			}
			
		}
		else if (strcmp(buf, "cd")==0)	/* function cd */
		{
			char *env;
			env = getenv("HOME");
			chdir(env);
			
			if(getcwd(buffer, sizeof(buffer)) != NULL) 
			{

				strcpy(buf,buffer);
			}
		}
		
		
	/* send results to client */
		nw = sendto(sd, buf, sizeof(buf), 0, (struct sockaddr *) &cli_addr, cli_addr_len);
		if (nw <= 0)
			exit(1);    /* send error */
		printf("server[%d]: send %s\n", i,buf);
	}
}

int main()
{
	int sd, nsd, cli_addr_len;  pid_t pid;
	struct sockaddr_in ser_addr, cli_addr; 
	char buf[BUFSIZE];

	/* turn the program into a daemon */
	daemon_init(); 
		 
	/* set up listening socket sd */
	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("server:socket");
		exit(1);
	} 

	/* build server Internet socket address */
	bzero((char *)&ser_addr, sizeof(ser_addr));
	ser_addr.sin_family = AF_INET;
	ser_addr.sin_port = htons(SERV_TCP_PORT);
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	/* note: accept client request sent to any one of the
			network interface(s) on this host. 
	*/

	/* bind server address to socket sd */
	if (bind(sd, (struct sockaddr *) &ser_addr, sizeof(ser_addr))<0)
	{
		perror("server bind");
		exit(1);
	}

	/* become a listening socket */
	listen(sd, 5);

	while (1) 
	{

	/* wait to accept a client request for connection */
		cli_addr_len = sizeof(cli_addr);
		nsd = accept(sd, (struct sockaddr *) &cli_addr, &cli_addr_len);
		if (nsd < 0)
		{
			if (errno == EINTR)   /* if interrupted by SIGCHLD */
				continue;
			perror("server:accept");
			exit(1);
		}
				   
	/* create a child process to handle this client */
		if ((pid=fork()) <0)
		{
			perror("fork");
			exit(1);
		}
		else if (pid > 0) 
		{ 
			close(nsd);
			continue; /* parent to wait for next client */
		}

	/* now in child, serve the current client */
		close(sd); 
		serve_a_client(nsd,cli_addr, cli_addr_len);
		exit(0);
	}
}
