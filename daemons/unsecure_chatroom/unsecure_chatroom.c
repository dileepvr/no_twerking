/*
** chatroom_server.c -- a stream socket chatroom server
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>


#define BACKLOG 10     // how many pending connections queue will hold

#define STDIN 0

#define MAXBUFLEN 256

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int handle_server_input(char *bufin, int buflen) {
  return 1;
}

int send_welcome(int fd, int rec_flag) {
  if (send(fd, "\t\t\t#SERVER: Welcome to Chatroom\n", 32, 0) == -1)
    { perror("\t\t\t#SERVER: send"); return 0;}
  if (rec_flag != 0) {
  if (send(fd, "\t\t\t#SERVER: Record_flag is ON\n", 30, 0) == -1)
    { perror("\t\t\t#SERVER: send"); return 0;}
  } else {
  if (send(fd, "\t\t\t#SERVER: Record_flag is OFF\n", 31, 0) == -1)
    { perror("\t\t\t#SERVER: send"); return 0;}
  }
  return 1;
}

int main(int argc, char *argv[])
{
    int listener, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN], buf[MAXBUFLEN], buf2[MAXBUFLEN], recordname[256];
    int rv, conno, numbytes;
    FILE *fileid;
    time_t raw_time;
    struct tm *ptr_ts;
    fd_set readfds, master;
    int fdmax, record_flag, iter, jter, numusers = 0, maxusers;
    struct timeval timeout;

    if (argc != 4) {
        fprintf(stderr,"usage: chatroom_server listen_port max_users record_flag\n");
        exit(1);
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    conno = 0; record_flag = strtol(argv[3],NULL,10);
    maxusers = atoi(argv[2]);
    printf("record_flag = %d\n",record_flag);
    FD_ZERO(&readfds); FD_ZERO(&master);
    memset(&hints, 0, sizeof hints);
    memset(buf, 0, sizeof(buf));
    memset(recordname, 0, sizeof(recordname));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    time(&raw_time); ptr_ts = localtime(&raw_time);    
    sprintf(recordname,"chatlogs/%s",asctime(ptr_ts));
    recordname[strlen(recordname)-1] = '\0'; strcat(recordname,"_chatroom.log");
    if(record_flag != 0) {
      if (fileid = fopen(recordname,"a")){
	fputs("########################################################\n",fileid);
	fputs("# Recording Started/Resumed on: ",fileid);
	fputs(asctime(ptr_ts),fileid);
	fputs("########################################################\n",fileid); 
	fflush(fileid);
      } else {
	record_flag = 0;
	printf("\t\t\t#SERVER:Can't open log file.\n");
      }
    }


    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("\t\t\t#SERVER: socket");
            continue;
        }

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("\t\t\t#SERVER: setsockopt");
            exit(1);
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            close(listener);
            perror("\t\t\t#SERVER: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "\t\t\t#SERVER: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(listener, BACKLOG) == -1) {
        perror("\t\t\t#SERVER: listen");
        exit(1);
    }



    FD_SET(STDIN, &master);
    FD_SET(listener, &master);
    fdmax = listener;
    
    printf("\t\t\t#SERVER: waiting for connections...\n");

    for(;;) {  // main accept() loop

      readfds = master;
      if (select(fdmax+1, &readfds, NULL, NULL, NULL) == -1) {
	perror("\t\t\t#SERVER: select");
	exit(4);
      }

      for(iter = 0; iter <= fdmax; iter++) {

	if(FD_ISSET(iter, &readfds)) {

	  if(iter == STDIN){
	    fgets(buf, sizeof(buf), stdin);
	    if(handle_server_input(buf, strlen(buf))==0) {
	      perror("\t\t\t#SERVER: Couldn't handle input.\n");
	    }
	    memset(buf, 0, sizeof(buf));
	  }
	  else if(iter == listener) {
	    sin_size = sizeof their_addr;
	    new_fd = accept(listener, (struct sockaddr *)&their_addr, &sin_size);
	    if (new_fd == -1) {
	      perror("\t\t\t#SERVER: accept");
	    } else {

	      inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
	      if(numusers < maxusers) {
		sprintf(buf,"\t\t\t#SERVER: got connection from %s\n", s);
		printf("%s",buf);
		numusers++;
		if(record_flag != 0) 
		  {
		    memset(buf2, 0, sizeof(buf2));		    
		    time(&raw_time); ptr_ts = localtime(&raw_time);
		    sprintf(buf2,"\t\t\t#SERVER: got connection from %s at %s", s, asctime(ptr_ts));
		    fputs(buf2,fileid);
		    fflush(fileid);
		  }
		memset(buf, 0, sizeof(buf));
		conno++;
		FD_SET(new_fd, &master); 
		if(new_fd > fdmax) fdmax = new_fd;
		send_welcome(new_fd, record_flag);
	      } else { // Too many users. Close connection.
		sprintf(buf,"\t\t\t#SERVER: rejecting connection from %s\n", s);
		printf("%s",buf);
		if(record_flag != 0) 
		  {
		    memset(buf2, 0, sizeof(buf2));		    
		    time(&raw_time); ptr_ts = localtime(&raw_time);
		    sprintf(buf2,"\t\t\t#SERVER: rejecting connection from %s at %s", s, asctime(ptr_ts));
		    fputs(buf2,fileid);
		    fflush(fileid);
		  }
		if (send(new_fd, "\t\t\t#SERVER: Chatroom is full! Sorry.\n", 38, 0) == -1)
		    { perror("\t\t\t#SERVER: send"); }
		close(new_fd);
	      }
	    }
	  } else {

	    if((numbytes = recv(iter, buf, MAXBUFLEN-1,0)) <= 0) {
	      if (numbytes == 0) {
		// connection closed
		printf("\t\t\t#SERVER: socket %d hung up\n", iter);
		numusers--;
		if(record_flag != 0) 
		  {
		    memset(buf2, 0, sizeof(buf2));		    
		    time(&raw_time); ptr_ts = localtime(&raw_time);
		    sprintf(buf2,"\t\t\t#SERVER: Socket %d: hung up at %s",iter, asctime(ptr_ts));
		    fputs(buf2,fileid);
		    fflush(fileid);
		  }
	      } else {
		perror("\t\t\t#SERVER: recv");
	      }
	      close(iter); // bye!
	      FD_CLR(iter, &master);
	      conno--;
	    } else {

	      // we got some data from a client
	      //	      send_to_all(fdmax, &master, iter, listener);
	      sprintf(buf2,"Socket %d: %s",iter, buf);	      
	      for(jter = 0; jter <= fdmax; jter++) {
		// send to everyone!
		if (FD_ISSET(jter, &master)) {
		  // except the listener and ourselves
		  if (jter == 0) {
		    printf("Socket %d: %s", iter, buf);
		    if(record_flag != 0) 
		      {
			fputs(buf2,fileid);
			fflush(fileid);
		      }
		  }
		  else if(jter != listener && jter != iter) {
		    //		    if (send(jter, buf, numbytes, 0) == -1) {
		    if (send(jter, buf2, strlen(buf2)+1, 0) == -1) {		    
		      perror("\t\t\t#SERVER: send");
		    }
		  }
		  
		}//if(FD_ISSET)
	      }	//for(jter)      
	      memset(buf, 0, sizeof(buf));

	    }
	    
	  }

	}// if(FD_ISSET) 
      }//for(iter)
    }//for(;;)

    fflush(fileid);
    fclose(fileid);
    return 0;
}


