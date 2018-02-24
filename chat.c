#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>
#include <time.h>

#define MAXBUF 80
#define MAXCLIENT 5
#define KAL_interval 1.5
#define KAL_count 5


int main(int argc, char*argv[]){
	int i, j, k, N, port, timeout, s, fromlen, CLIENT_LIMIT,count[MAXCLIENT];
	int newsock[MAXCLIENT+2];
	char buf[MAXBUF], input[MAXBUF],KALusers[MAXCLIENT][200];
	char users[MAXCLIENT][10], to[MAXCLIENT][MAXCLIENT][10];
	struct pollfd pfd[MAXCLIENT+1];
	struct sockaddr_in sin, from;
	time_t usertime[MAXCLIENT], seconds, KAL[MAXCLIENT];

	port=strtol(argv[2],NULL,10);

	for(i=0;i<MAXCLIENT;i++){
		KAL[i]=time(NULL);//for sending keepalive messages
	}

	if(strcmp(argv[1],"-s")==0){//server

		for(i=0;i<MAXCLIENT;i++){
			count[i]=0;
			memset(users[i],0,sizeof(users[i]));
			memset(KALusers[i],0,sizeof(KALusers[i]));
			for(j=0;j<MAXCLIENT;j++){
				memset(to[i][j],0,sizeof(to[i][j]));
			}
		}
		for(i=0;i<MAXCLIENT+2;i++)	newsock[i]=-1;

		CLIENT_LIMIT=strtol(argv[3],NULL,10);//get client limit

		if((s=socket(AF_INET,SOCK_STREAM,0))<0){
			fprintf(stderr,"%s:socket\n",argv[0]);
			exit(1);	
		}
		
		sin.sin_family=AF_INET;
		sin.sin_addr.s_addr=htonl(INADDR_ANY);
		sin.sin_port=htons(port);

		if(bind(s,(struct sockaddr*)&sin,sizeof(sin))<0){
			fprintf(stderr,"%s:bind\n",argv[0]);
			exit(1);
		}
			
		listen(s,0);
		//master socket and standard input
		timeout=0;
		pfd[0].fd=s;	pfd[1].fd=STDIN_FILENO;
		pfd[0].events=pfd[1].events=POLLIN;
		pfd[0].revents=pfd[1].revents=0;
	
		
		seconds=time(NULL); //for the activity report
		
		printf("Chat server begins [port= %d] [nclient= %d]\n",port,CLIENT_LIMIT);	
		N=CLIENT_LIMIT+2;
		while(1){
			memset(buf,0,MAXBUF);

			//activity report
			//=================================================
			if(time(NULL)-seconds>10){//every 10 seconds
				seconds=time(NULL);
				puts("\nactivity report:");
				for(i=0;i<CLIENT_LIMIT;i++){
					if(strlen(users[i])!=0){
						printf("'%s' [sockfd= %d]: %s",users[i],i+2,ctime(&usertime[i]));
					}
				}
				for(i=0;i<MAXCLIENT;i++){
					if(strlen(KALusers[i])!=0){
						printf("%s",KALusers[i]);
						memset(KALusers[i],0,MAXBUF);
					}
				}
			}
			//===================================================

			poll(pfd,N,timeout);
			if(pfd[1].revents & POLLIN){
				fgets(buf,sizeof(buf),stdin);
                                if(strcmp(buf,"exit\n")==0) break;
			}
			//polls each socket for any reads
			for(i=2;i<CLIENT_LIMIT+2;i++){		
				if(pfd[i].revents & POLLIN){
					if(read(newsock[i],buf,MAXBUF)>0){ 
		
						//commands
						if(strncmp(buf,"close",5)==0){
							for(j=0;j<CLIENT_LIMIT;j++){//remove users' recipient array
								memset(to[i-2][j],0,sizeof(to[i-2][j]));
							}
							//remove username from any of the other users' recipients array
							//----------------------------------------------------------------
							for(j=0;j<CLIENT_LIMIT;j++){
								for(k=0;k<CLIENT_LIMIT;k++){
									if(strcmp(users[i-2],to[j][k])==0){
										memset(to[j][k],0,sizeof(to[j][k]));
									}
								}		
							}
							//----------------------------------------------------------------
							memset(users[i-2],0,sizeof(users[i-2]));// remove username from array of users
							write(newsock[i],"[server] done",strlen("[server] done"));	
							close(newsock[i]);
							newsock[i]=-1;//set socket file descriptor to unused (-1)
						}
						else if(strncmp(buf,"who",3)==0){
							time(&usertime[i-2]);//update time
							memset(input,0,MAXBUF);
							sprintf(input,"[server]: Current users:");
							for (j=0;j<CLIENT_LIMIT;j++){
								if(strlen(users[j])!=0){
									strcpy(buf,input);
									sprintf(buf,"%s %s,",input,users[j]);
									strcpy(input,buf);
								}
							}
							buf[strlen(buf)-1]='\0';
							write(newsock[i],buf,sizeof(buf));
						}
						else if(strncmp(buf,"to ",3)==0){
							time(&usertime[i-2]);//update time
							memset(input,0,MAXBUF);
							for(j=0;j<MAXBUF-3;j++){
								input[j]=buf[j+3];
							}
							memset(buf,0,MAXBUF);
							strcpy(buf,"[server] recipients added:");
							char *name=strtok(input," ");
							while(name){
								for(j=0;j<CLIENT_LIMIT;j++){
									if(strcmp(name,users[j])==0 && strlen(users[j])!=0){
										for(k=0;k<CLIENT_LIMIT;k++){
											if(strlen(to[i-2][k])==0){
												strcpy(to[i-2][k],name);
												break;			
											}
										}
										sprintf(buf,"%s %s,",buf,name);
									}
								}
								name=strtok(NULL," ");
							}
							buf[strlen(buf)-1]='\0';	
							write(newsock[i],buf,strlen(buf));
						}
						else if(strncmp(buf,"< ",2)==0){
							time(&usertime[i-2]);//update time
							j=0;
							//check if empty
							//----------------------------------------
							while(j<CLIENT_LIMIT){
								if(strlen(to[i-2][j])!=0)	break;
								j++;
							}
							if(j==CLIENT_LIMIT){	
								write(newsock[i],"[server] recipient list empty",strlen("[server] recipient list empty"));
							}
							//----------------------------------------
							else{
								for(j=0;j<MAXBUF-2;j++)	input[j]=buf[j+2];	
								sprintf(buf,"\n[%s] %s",users[i-2],input);
								buf[strlen(buf)]='\0';
								for(j=0;j<CLIENT_LIMIT;j++){
									if(strlen(to[i-2][j])!=0){
										for(k=0;k<=CLIENT_LIMIT;k++){
											if(strcmp(to[i-2][j],users[k])==0){
												write(newsock[k+2],buf,strlen(buf));
											}		
										}
									}
								}
							}
						}
						//reset count after receiving keep alive message
						else if(strncmp(buf,"0x6",3)==0)	count[i-2]=0;	
					}	
				}
				//increase count by 1 every KAL_interval
				if(time(NULL)-KAL[i-2]>=KAL_interval && newsock[i]!=-1){
					KAL[i-2]=time(NULL);
					count[i-2]++;

					if(count[i-2]==5){//close if keep alive message was not received 5 times in a row
						for(j=0;j<CLIENT_LIMIT;j++){//remove users' recipient array
							memset(to[i-2][j],0,sizeof(to[i-2][j]));
						}
						//remove username from any of the other users' recipients array
						//----------------------------------------------------------------
						for(j=0;j<CLIENT_LIMIT;j++){
							for(k=0;k<CLIENT_LIMIT;k++){
								if(strcmp(users[i-2],to[j][k])==0){
									memset(to[j][k],0,sizeof(to[j][k]));
								}
							}		
						}
						//----------------------------------------------------------------
						write(newsock[i],"[server] done",strlen("[server] done"));	
						close(newsock[i]);
						newsock[i]=-1;//set socket file descriptor to unused (-1)

						count[i-2]=0;
						for(j=0;j<MAXCLIENT;j++){
							if(strlen(KALusers[j])==0){
								time(&usertime[i-2]);
								sprintf(KALusers[j],"'%s' [sockfd= %d]: loss of keepalive messages detected at %s",users[i-2],i,ctime(&usertime[i-2]));
								KALusers[j][strlen(KALusers[j])-1]='\0';
								sprintf(KALusers[j],"%s, connection closed\n",KALusers[j]);
								break;
							}
						}
						memset(users[i-2],0,sizeof(users[i-2]));// remove username from array of users
					}
				}
			}
			//new connection
			//==========================================================================
			if(pfd[0].revents & POLLIN){
				for(i=2;i<CLIENT_LIMIT+2;i++){
					fromlen=sizeof(from);			
					//if client limit reached	
					if((newsock[i]!=-1) && i==CLIENT_LIMIT+1){
						// newsock[1] used as a socket for letting clients
						//know that the server is at its client limit
						newsock[1]=accept(s,(struct sockaddr*)&from,&fromlen);
						write(newsock[1],"f",1);
						close(newsock[1]);
					}

					if(newsock[i]==-1){//file descriptor not in use (-1)	
						//accept a new connection
						newsock[i]=accept(s,(struct sockaddr*)&from,&fromlen);
					
						write(newsock[i],"[server] connected\n",strlen("[server] connected\n"));
					
						read(newsock[i],buf,MAXBUF);
						
						//get username from 'open username'
						//---------------------------------
						for(j=0;j<sizeof(buf)-5;j++){
							input[j]=buf[j+5];
						}				
						input[strlen(input)]='\0';
						//----------------------------------

						memset(buf,0,MAXBUF);
						for(j=0;j<CLIENT_LIMIT;j++){//check username
							if(strcmp(users[j],input)==0){//username taken
								write(newsock[i],"[server] username taken",strlen("[server] username taken"));
								close(newsock[i]);
								newsock[i]=-1;
								break;
							}
							//username not in list of connected users
							if(strcmp(users[j],input)!=0 && j==CLIENT_LIMIT-1){
								strcpy(users[i-2],input);
								sprintf(buf,"[server] User '%s' logged in",input);
								write(newsock[i],buf,strlen(buf));

								//add to file descriptors to poll
								pfd[i].fd=newsock[i];
								pfd[i].events=POLLIN;
								pfd[i].revents=0;
								time(&usertime[i-2]);//set time of login
							}
						}		
					 	break;
					}
				}
			
			}
			//========================================================================
		}
		for(i=2;i<CLIENT_LIMIT+2;i++) close(newsock[i]);
		close(s);
	}
	else{  //client
		int connected=0;
		struct hostent *hp;
		
		hp=gethostbyname(argv[3]);

		sin.sin_family=AF_INET;
		sin.sin_port=htons(port);
		sin.sin_addr.s_addr=*((unsigned long *)hp->h_addr);
	
		timeout=0;	
		pfd[0].fd=STDIN_FILENO;
		pfd[0].events=POLLIN;
		pfd[0].revents=0;
			
		printf("Chat client begins (server '%s' [%s], port %d)\n",argv[3],inet_ntoa(sin.sin_addr),port);
		printf("a3chat_client: ");
		fflush(stdout);
		N=1;
		while(1){
			poll(pfd,N,timeout);
			memset(buf,0,MAXBUF);
			if(pfd[0].revents & POLLIN){
				fgets(buf,MAXBUF,stdin);
				buf[strlen(buf)-1]='\0';
				if(strncmp(buf,"open",4)==0){
				
					if(connected==1)	printf("already connected!\n");
					
					else if((s=socket(AF_INET,SOCK_STREAM,0))>=0){
						if(connect(s,(struct sockaddr *)&sin,sizeof(sin))>=0){			
							if(read(s,input,MAXBUF)>0){
								if(strncmp(input,"f",1)==0){	
									printf("server full, try again later\n");
									close(s);
								}
								else{
									printf("%s",input);
									write(s,buf,strlen(buf));	
									memset(buf,0,MAXBUF);
									read(s,buf,MAXBUF);
									if(strncmp(buf,"[server] username taken",sizeof("[server] username taken"))==0){
										printf("%s\nserver disconnected\n",buf);
										close(s);
									}
									else{
										printf("%s\n",buf);
										connected=1;
										pfd[1].fd=s;
										pfd[1].events=POLLIN;
										pfd[1].revents=0;
										N=2;
									}
								}
							}
							else	fprintf(stderr,"%s: converting to FILE*\n",argv[0]);
						}
						else	fprintf(stderr,"%s:connect\n",argv[0]);	
					}

					else	fprintf(stderr,"%s:socket\n",argv[0]);
				
					printf("a3chat_client: ");	
				}
				else if(strcmp(buf,"exit")==0) break;
				else if(strcmp(buf,"close")==0){ 
					write(s,buf,strlen(buf));
					read(s,buf,MAXBUF);
					printf("%s\na3chat_client: ",buf);
					close(s);
					connected=0;
					N=1;
				}
				else if (connected==0)	printf("not connected\na3chat_client: ");
				else	write(s,buf,strlen(buf));
			}
			if(connected==1){
				memset(buf,0,MAXBUF);
				if((pfd[1].revents & POLLIN)){
					if(read(s,buf,MAXBUF)>0){ 
						printf("%s\na3chat_client: ",buf);
					}
				}
				memset(buf,0,MAXBUF);
				//send keep alive messages every KAL_interval seconds
				if(time(NULL)-KAL[0]>=KAL_interval){
					KAL[0]=time(NULL);
					write(s,"0x6",strlen("0x6"));
				}
			}
			fflush(stdout);
		}
	}
	return 0;
}
