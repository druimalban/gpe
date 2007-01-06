#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/poll.h>
//#include <math.h>


#define VERSION "0.0.12"
#define MIN(a,b)	(((a)<(b)) ? (a):(b))

#define safe_strcpy(a,b) {siof=sizeof(a);a[siof-1]=0;strncpy(a,b,siof-1);}
int siof;


int fd_out,fd_in;
int label=0;
char imap_buf[2000000];
char *imap_buf_ptr;
int msgs[5000];
typedef char str32t[32];
str32t  msg_ids[5000];
int debug=0;
int verbose=0;
char rcv_buf[100000];
DIR *dir;
FILE *fl;
struct dirent * direntptr;

struct {
	char server[50],user[50],pass[50],since[50],prefix[50];
	int port,size;
	int ssl,no_del,remove,cleandir;
	int pop3,smtp;
	float timeout;
} options;

#define imap_buf_make_ready()	imap_buf_ptr=imap_buf

inline char file_exists(char *s)
{
	struct stat st;
	char c;
	c=(!stat(s,&st));
	if (debug>1) printf("file_exists called with '%s', returning: %d\n",s,(int)c);
	return c;
}

char is_hexchar(char c)
{
	if (c>='0' && c<='9') return 1;
	if (c>='a' && c<='z') return 1;
	if (c>='A' && c<='Z') return 1;
	return 0;
}


inline char starts_with(char *str, char *substr)
{
        return (strstr(str,substr)==str);
}

#define extract_next_email_adress()	extract_first_email_adress(0)
char *extract_first_email_adress(char *buf)
{
	static char *sp=0;
	char inside_quote;
	char found_ampersand;
	char found_braket;
	int i;

	if (buf) sp=buf; // find first
	
	inside_quote=0;
	found_braket=found_ampersand=0;
	for(i=0;sp[i];i++)
	{
		if (sp[i]=='"')
		{
			if (inside_quote)
			{
				sp+=i+1;
				i=-1;
				found_braket=found_ampersand=0;
				inside_quote=0;
			} else {
				inside_quote=1;
			}
			continue;
		}
		else if (inside_quote) continue;
		if (sp[i]=='@')	{found_ampersand++;continue;}
		if (sp[i]=='<') { if (found_braket) return 0; else found_braket=1;}
		if (sp[i]=='>') { if (found_braket!=1) return 0; else found_braket=2;}
		if (sp[i]==' ') 
		{
			if (found_braket==1) return 0;
			if (!found_braket)
			{
				sp+=i+1;
				i=-1;
				found_braket=found_ampersand=0;
			}
			continue;
		}
		else if (sp[i]==','||sp[i]==';'||!sp[i+1])
		{
			if (!found_ampersand) return 0;
			if (!(sp[i]==','||sp[i]==';')) i++;
			sp[i]=0;
			if (index(sp,' ')) *index(sp,' ')=0;
			sp+=i+1;
			return (sp-i-1);
		}
	}
	return 0;
}


inline char ends_with(char *str, char *substr)
{
        return ((strstr(str,substr)+strlen(substr))==(str+strlen(str)));
}

char lowcase(char c)
{
	if (c>='A' && c<='Z') c+='a'-'A';
	return c;
}

void lowstr(char *s)
{
	int i;

	for(i=0;i<strlen(s);i++)
		s[i]=lowcase(s[i]);
}

int read_with_timeout(int fd, void *buf, size_t count)
{
	struct pollfd pfd;
	char c;

	pfd.fd=fd;
	pfd.events=POLLIN;
	pfd.revents=0;

	fcntl(fd,O_NONBLOCK);
	if (debug>1)
	{
		printf("polling ... ");
		fflush(stdout);
	}
	c=poll(&pfd,1,(size_t)(options.timeout));
	if (debug>1) printf("=%d (revents=%d)\n",(int)c,(int)pfd.revents);
	if (!c) return 0;
	return (read(fd,buf,count));
}

int tilde(char *file,char * exp)
{
  *exp = '\0';
  if (file)
  {
    if (*file == '~')
    {
      char user[128];
      struct passwd *pw = NULL;
      int i = 0;
      
      user[0] = '\0';
      file++;
      while (*file != '/' && i < sizeof(user))
        user[i++] = *file++;
      user[i] = '\0';
      if (i == 0)
      {
        char *login = (char *) getlogin();
        
        if (login == NULL && (pw = getpwuid(getuid())) == NULL)
          return (0);
        if (login != NULL)
          safe_strcpy(user, login);
      }
      if (pw == NULL && (pw = getpwnam(user)) == NULL)
        return (0);
      safe_strcpy(exp, pw->pw_dir);
    }
    strcat(exp, file);
    return (1);
  } return (0);
}


void error(char *s1, char *s2)
{
	fprintf(stderr,"ERROR: %s: %s\n",s1,s2);
	exit(1);
}

void conn (char *host,int port)
{
int p2c_fds[2];
int c2p_fds[2];
int i;
char s[100];
char *args[5];

struct sockaddr_in  sa;        /*    Internet address struct   */
struct hostent* hen;           /*    host-to-IP translation    */
int sock_fd;


	if (debug>1) printf("conn called with %s:%d\n",host,port);
	if (options.ssl)
	{
		// start by forking. child does stunnel
		pipe(p2c_fds);
		pipe(c2p_fds);
		signal(SIGCHLD,SIG_IGN); //prevent zombies
		i=fork();
		if (!i) // child
		{
			sprintf(s,"stunnel%c-c%c-r%c%s:%d",0,0,0,host,port);
			args[0]=s;
			args[1]=s+8;
			args[2]=s+11;
			args[3]=s+14;
			args[4]=0;
			dup2(p2c_fds[0],0);
			dup2(c2p_fds[1],1);
			close(p2c_fds[1]);
			close(c2p_fds[0]);
			execv("/usr/sbin/stunnel",args);
			perror("\"/usr/sbin/stunnel\"");
			exit(1);
		} else { //parent
			close(c2p_fds[1]);
			close(p2c_fds[0]);
			fd_out=p2c_fds[1];
			fd_in=c2p_fds[0];
		}
	} else { // ssl==0
		 hen=gethostbyname(host);
		 if (!hen) {perror("gethostbyaddr");exit(1);}
		 memset(&sa, 0, sizeof(sa));
		 sa.sin_family = AF_INET;
		 sa.sin_port = htons(port);
		 memcpy(&sa.sin_addr.s_addr, hen->h_addr_list[0], hen->h_length);
		 sock_fd=socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
		 if (sock_fd<0) {perror("socket");exit(1);}

		 if (connect(sock_fd,(struct sockaddr *)&sa,sizeof(sa))<0)
		 {perror("connect");exit(1);}
		 fd_in=fd_out=sock_fd;
	}
}

void send2server(char *s)
{
	char s2[205];

	if (options.pop3||options.smtp) sprintf(s2,"%s\r\n",s);
	else sprintf(s2,"From:%d %s\r\n",++label,s);
	if (debug>1) {
		printf("DEBUG: sending: %s",s2);
	}
	write(fd_out,s2,strlen(s2));
	fsync(fd_out);

}

void imap_gets(char *s, int size)
{
	static int len=0;
	static char buf[10000];
	int i,j;
	char *p;


	*s=0;
	j=0;
	do 
	{
		if ((p=index(buf,'\n'))==NULL ) //|| !index)
		{
			buf[len]=0;
			i=read_with_timeout(fd_in,buf+len,sizeof(buf)-2-len);
			if (!i && !j ) return;
			j+=i;
			len+=i;
			buf[len]=0;
		}
		p=MIN(index(buf,'\r'),index(buf,'\n'));
		if (!p) p=index(buf,'\n');
		if (!p && debug>1)
		{
			printf("no CR or LF found, retrying ?\n");
		}
	} while (i && !p && (strlen(buf)<(sizeof(buf)-2)));
	i=j;
	if (!p)
	{
		fprintf(stderr,"WARNING: no CR or LF found, probably line too long\n");
		p=buf+strlen(buf)-2;
	}
	strncpy(s,buf,MIN(p-buf,size-1));
	s[MIN(p-buf,size-2)]='\n';
	s[MIN(p-buf+1,size-1)]=0;
	//p++;
	if (*p=='\r') p++;
	if (*p=='\n') p++;
	//while (*p=='\n' || *p== '\r' ) p++;
	safe_strcpy(buf,p);
	len-=p-buf;
}


#define smtp_rcv_ok	pop_rcv_ok
int pop_rcv_ok(char *section, char bye_if_err,int waitfordata)
{
char truncated;
char waitfordot;
int v;

	imap_buf[0]=0;
	truncated=0;
	waitfordot=0;
	while (1)
	{
		rcv_buf[0]=0;
		imap_gets(rcv_buf,sizeof(rcv_buf));
		if (debug>2) {
			printf("DEBUG: rcv_buf: %s",rcv_buf[0] ? rcv_buf:"\n");
		}
		if (!rcv_buf[0]) goto fail;
		if (strlen(imap_buf)+strlen(rcv_buf)+2>=sizeof(imap_buf))
		{
			if ( !truncated)
			{
				truncated=1;
				fprintf(stderr," Warning: %s: rcv_buf too small, truncated ! \n",section);
			}
		} else strcat(imap_buf,rcv_buf);
		if (options.smtp)
		{
			v=0;
			sscanf(rcv_buf,"%d ",&v);
			if (v)
			{
				if (debug>1) printf("got %d, expecting %d\n",v,waitfordata);
				if (v!=waitfordata) goto fail;
				return 1;
			}
		} else {
			if (!waitfordot && starts_with(rcv_buf,"+OK"))
			{
				if (!waitfordata) return 1;
				waitfordot=1;
	
			}
			if (starts_with(rcv_buf,"-ERR")) goto fail;
			if (waitfordot && starts_with(rcv_buf,".\n")) return 1;
		}
	}
fail:
	if (bye_if_err) error(section,rcv_buf);
	return 0;
}

int rcv_ok(char *section, char bye_if_err)
{
int j;
char s2[100];
char ast;
char truncated;
char *lp;
	
	if (options.pop3) error("rcv_ok","pop3 mode");
	imap_buf[0]=0;
	ast=0;
	truncated=0;
	for(j=0;j<300;j++)
	{
		lp=imap_buf+strlen(imap_buf);
		imap_gets(rcv_buf,sizeof(rcv_buf));
		if (debug>2) {
			printf("DEBUG: rcv_buf: %s",rcv_buf[0] ? rcv_buf:"\n");
		}
		if (!rcv_buf[0]) goto fail;
		if (rcv_buf[0]=='*' || ast) 
		{
			ast=1;
			if (strlen(imap_buf)+strlen(rcv_buf)+2>=sizeof(imap_buf))
			{
				if ( !truncated)
				{
					truncated=1;
					fprintf(stderr," Warning: %s: rcv_buf too small, truncated ! \n",section);
				}
				//error(section,"rcv_buf too small");
			}	
			else
				strcat(imap_buf,rcv_buf);
			j--;
			if (rcv_buf[0]=='*') continue;
		}
		sprintf(s2,"From:%d ",label);
		if (0!=strncmp(s2,rcv_buf,strlen(s2)))
		{
			//imap_buf[0]=0;
			continue;
		}
		sprintf(s2,"From:%d OK",label);
		if (0!=strncmp(s2,rcv_buf,strlen(s2))) goto fail;
		if (debug>1) {
			printf("DEBUG: got %d bytes:\n%sDEBUG: end\n",strlen(imap_buf),imap_buf);
			printf("DEBUG: got what i wanted: %s",rcv_buf);
		}
		*lp=0;
		return 1;
	}
fail:
	if (bye_if_err) error(section,rcv_buf);
	return 0;
}



void imap_buf_get(char *buf, int bufsize)
{
char *end;
char c;

	*buf=0;

	if (!imap_buf_ptr) return;
	end=index(imap_buf_ptr,'\n');
	if (!end) {imap_buf_ptr=0;return;}
	end++;
	c=*end;
	*end=0;
	if ((end-imap_buf_ptr)>bufsize-2) error("imap_buf_get: line too long",imap_buf_ptr);
	safe_strcpy(buf,imap_buf_ptr);
	*end=c;
	imap_buf_ptr=end;
}

int mkdir_minus_p(char *s)
{
	char s2[100];
	char *sp;
	int i;

	safe_strcpy(s2,s);
	i=mkdir(s2,S_IREAD|S_IWRITE|S_IEXEC);
	if (!i) return 0; // created successfully
	if (errno==EEXIST) return 0; // already exists (dir or file)
	while (0!=(sp=rindex(s2,'/')))
	{
		*sp=0;
		if (mkdir_minus_p(s2)) return 1;
		return mkdir(s,S_IREAD|S_IWRITE|S_IEXEC);
	}
	return 1;
}
FILE *openfile(char *s,char read)
{
	char s2[100];
	char *sp;
	FILE *fl;

	if (debug>1) printf("openfile called with name=%s, read=%d\n",s,read);
	if (read)
	{
		if (NULL==(fl=fopen(s,"r"))) error("fopen",s);
		return fl;
	}
	safe_strcpy(s2,s);
	if (0!=(sp=rindex(s2,'/')))
	{
		*sp=0;
		if (mkdir_minus_p(s2)) error("mkdir",s2);
	}
	if (NULL==(fl=fopen(s,"w"))) error("fopen",s);
	return fl;
}

int do_smtp()
{
char s[200];
char prefix[200];
char s2[20000];
char *sp;
char connected;
char from_str[1000],to_str[1000],cc_str[1000],bcc_str[1000],date_str[1000],subject_str[1000];

	connected=0;
	if (verbose) printf("looking for messages to send\n");
	sprintf(s,"%s/OUTBOX",options.prefix);
	snprintf(prefix,sizeof(prefix),"%s/OUTBOX",options.prefix);
	dir=opendir(prefix);
	if (!dir) {perror(prefix);exit(0);} 
	direntptr=(void *)1;
	do
	{
		direntptr=readdir(dir);
		if (direntptr)
		{
			if (ends_with(direntptr->d_name,".head"))
			{
				sprintf(s,"%s/%s",prefix,direntptr->d_name);
				*rindex(s,'.')=0;
				strcat(s,".body");
				if (!file_exists(s))
				{
					printf("Warning, only header found : %s\n",direntptr->d_name);
					continue;
				} else {
					*rindex(s,'.')=0;
					strcat(s,".head");
					fl=openfile(s,1);
					*from_str=*to_str=*cc_str=*bcc_str=*date_str=*subject_str=0;
					do
					{
						*s2=0;
						fgets(s2,sizeof(s2),fl);
						if (s2[strlen(s2)-1]=='\n') s2[strlen(s2)-1]=0;
						if (s2[strlen(s2)-1]=='\r') s2[strlen(s2)-1]=0;
						if (starts_with(s2,"From: ")) safe_strcpy(from_str,s2+6);
						if (starts_with(s2,"To: ")) safe_strcpy(to_str,s2+4);
						if (starts_with(s2,"Cc: ")) safe_strcpy(cc_str,s2+4);
						if (starts_with(s2,"Bcc: ")) safe_strcpy(bcc_str,s2+5);
						if (starts_with(s2,"Date: ")) safe_strcpy(date_str,s2+6);
						if (starts_with(s2,"Subject: ")) safe_strcpy(subject_str,s2+9);
					} while (*s2);
					fclose(fl);
					if (debug)
					{
						if (*from_str)	printf("From:    %s\n",from_str);
						if (*to_str)	printf("To:      %s\n",to_str);
						if (*cc_str)	printf("Cc:      %s\n",cc_str);
						if (*bcc_str)	printf("Bcc:     %s\n",bcc_str);
						if (*date_str)	printf("Date:    %s\n",date_str);
						if (*subject_str) printf("Subject: %s\n",subject_str);
					}

					if (!connected)
					{
						if (verbose) printf("Connecting to server\n");
						conn(options.server,options.port);
						smtp_rcv_ok("conn",1,220);
						send2server("helo localhost");
						smtp_rcv_ok("helo",1,250);
						connected=1;
					}
					sp=extract_first_email_adress(from_str);
					if (!sp) 
					{
						printf("Error: no/illeagle 'From' field: %s\n",from_str);
						continue;
					}
					sprintf(s2,"mail from: %s",sp);
					send2server(s2);
					if (!smtp_rcv_ok(s2,0,250))
					{
						printf("error on '%s'\n",s2);
						goto do_rset;
					}
					sp=extract_first_email_adress(to_str);
					if (!sp) 
					{
						printf("Error: no/illeagle 'To' field: %s\n",from_str);
						goto do_rset;
					}
					do
					{
						sprintf(s2,"rcpt to: %s",sp);
						send2server(s2);
						if (!smtp_rcv_ok(s2,0,250))
						{
							printf("error on '%s'\n",s2);
							goto do_rset;
						}
						sp=extract_next_email_adress();
					} while(sp);
					sp=extract_first_email_adress(cc_str);
					if (sp) do
					{
						sprintf(s2,"rcpt to: %s",sp);
						send2server(s2);
						if (!smtp_rcv_ok(s2,0,250))
						{
							printf("error on '%s'\n",s2);
							goto do_rset;
						}
						sp=extract_next_email_adress();
					} while(sp);
					sp=extract_first_email_adress(bcc_str);
					if (sp) do
					{
						sprintf(s2,"rcpt to: %s",sp);
						send2server(s2);
						if (!smtp_rcv_ok(s2,0,250))
						{
							printf("error on '%s'\n",s2);
							goto do_rset;
						}
						sp=extract_next_email_adress();
					} while(sp);
					safe_strcpy(s2,"data");
					send2server(s2);
					if (!smtp_rcv_ok(s2,0,354))
					{
						printf("error on '%s'\n",s2);
						goto do_rset;
					}
					if (verbose>1)
					{
						printf("#");
						fflush(stdout);
					}
					fl=openfile(s,1);
					do
					{
						*s2=0;
						fgets(s2,sizeof(s2),fl);
						if (*s2)
						{
							if (s2[strlen(s2)-1]=='\n') s2[strlen(s2)-1]=0;
							if (s2[strlen(s2)-1]=='\r') s2[strlen(s2)-1]=0;
							if (!starts_with(s2,"Bcc: ")) send2server(s2);
						}
					} while (*s2);
					fclose(fl);
					send2server("X-Mailer: emailsync(" VERSION ")");
					send2server("");
					*rindex(s,'.')=0;
					strcat(s,".body");
					fl=openfile(s,1);
					do
					{
						*s2=0;
						fgets(s2,sizeof(s2),fl);
						if (*s2)
						{
							if (s2[strlen(s2)-1]=='\n') s2[strlen(s2)-1]=0;
							if (s2[strlen(s2)-1]=='\r') s2[strlen(s2)-1]=0;
							if (strcmp(s2,".")!=0) send2server(s2);
							*s2=1;
						}
					} while (*s2);
					fclose(fl);
					send2server(".");
					smtp_rcv_ok(".",1,250);
					if (!options.no_del) unlink(s);
					*rindex(s,'.')=0;
					strcat(s,".head");
					if (!options.no_del) unlink(s);
					continue;
do_rset:
					send2server("rset");
					smtp_rcv_ok(s2,1,250);
				}
			}
		}
	} while (direntptr);


	//printf("Currently, smtp is not supported\n");
	return 0;

}

int main(int argc, char **argv)
{
char s[100],s2[100],folder[100];
char *sp, *sp2;
int i,j,num_msgs,size,get_body;
float flt;
char flag;
char action;


	bzero(&options,sizeof(options));
	safe_strcpy(options.prefix,"~/.emailsync/");
	options.cleandir=1;
	options.timeout=30000; // 30 second timeout between reads
	argc--;
	i=argc;
	sp=*argv;
	while (argc)
	{
		argc--;
		argv++;
		if (debug>1) {
			printf("DEBUG: arg: %s\n",argv[0]);
		}
		if (starts_with(argv[0],"server=")) {safe_strcpy(options.server,index(argv[0],'=')+1);}
		else if (starts_with(argv[0],"user=")) {safe_strcpy(options.user,index(argv[0],'=')+1);}
		else if (starts_with(argv[0],"pass=")) {safe_strcpy(options.pass,index(argv[0],'=')+1);}
		else if (starts_with(argv[0],"since=")) {safe_strcpy(options.since,index(argv[0],'=')+1);}
		else if (starts_with(argv[0],"prefix=")) {safe_strcpy(options.prefix,index(argv[0],'=')+1);}
		else if (starts_with(argv[0],"ssl=")) sscanf(index(argv[0],'=')+1,"%d",&options.ssl);
		else if (starts_with(argv[0],"cleandir=")) sscanf(index(argv[0],'=')+1,"%d",&options.cleandir);
		else if (starts_with(argv[0],"remove=")) sscanf(index(argv[0],'=')+1,"%d",&options.remove);
		else if (starts_with(argv[0],"no_del=")) sscanf(index(argv[0],'=')+1,"%d",&options.no_del);
		else if (starts_with(argv[0],"port=")) sscanf(index(argv[0],'=')+1,"%d",&options.port);
		else if (starts_with(argv[0],"size=")) sscanf(index(argv[0],'=')+1,"%d",&options.size);
		else if (starts_with(argv[0],"debug=")) sscanf(index(argv[0],'=')+1,"%d",&debug);
		else if (starts_with(argv[0],"verbose=")) sscanf(index(argv[0],'=')+1,"%d",&verbose);
		else if (starts_with(argv[0],"timeout=")) sscanf(index(argv[0],'=')+1,"%f",&options.timeout);
		else if (starts_with(argv[0],"proto=pop")) options.pop3=1;
		else if (starts_with(argv[0],"proto=imap")) options.pop3=0;
		else if (starts_with(argv[0],"proto=smtp")) options.smtp=1;
		else if (starts_with(argv[0],"-h")) i=0;
		else if (starts_with(argv[0],"--help")) i=0;
		else error("unknown argument",argv[0]);
	}
	if (!i)
	{
		printf("emailsync version " VERSION "\n"
			"By Erez Doron <erez@savan.com>\n"
			"Licenced under the GPL\n\n"
			"Syntax: %s <args> [<optional args>]\n\n"
			"ARGS:\n"
			"server=<server-name-or-ip>\n"
			"user=<username>\n"
			"pass=<password>\n\n"
			"OPTIONAL ARGS:\n"
			"proto=pop3|imap|smtp\n"
			"port=<port>\n"
			"ssl=<1 for imaps, 0 for imap>\n"
			"since=<date>					(e.g. 1-mar-2002)\n"
			"size=<max msg size in bytes>\n"
			"debug=<0 1 2 or 3>\n"
			"timeout=<milli-seconds>\n"
			"prefix=<directory where to write mail to>\n"
			"verbose=<0 1 or 2>\n"
			"remove=1			(remove msg body if too big)\n"
			"no_del=1			( do not delete msgs from outbox after send)\n"
			"cleandir=1			(remove old messages from disk)\n"
			"\n\n"
			"DEFAULTS:\n"
			"by default, all parameters take the value of zero\n"
			"except cleandir which is 1, timeout (10,000 milisec)\n"
			"and prefix which is \"~/.emailsync/\"\n"
			"port by default is 143 for ssl==0 or 993 for ssl==1 (proto=imap),\n"
			"                   110 for ssl==0 or 995 for ssl==1 (proto=pop3),\n"
			"                    25 for ssl==0 or 465 for ssl==1 (proto=smtp)\n"
			"default proto is imap\n"
			"\n",sp);
		exit(1);
	}
	

	if (verbose) printf("emailsync version " VERSION "\n"
			"By Erez Doron <erez@savan.com>\n"
			"Licenced under the GPL\n\n");
	if (!tilde(options.prefix,s)) error(options.prefix,"illegle prefix");
	safe_strcpy(options.prefix,s);
	if (*options.prefix) if (options.prefix[strlen(options.prefix)-1]=='/') options.prefix[strlen(options.prefix)]=0;
	if (options.port==0)
	{
		if (options.smtp) options.port= options.ssl ? 465:25;
		else if (options.pop3) options.port= options.ssl ? 995:110;
		else  options.port= options.ssl ? 993:143;
	}
	if (options.smtp) return do_smtp();
	if (verbose) printf("Connecting to server\n");
	conn(options.server,options.port);
	if (verbose) printf("Logging in\n");
	if (options.pop3) 
	{
		pop_rcv_ok("connect",1,0);
		sprintf(s,"user %s",options.user);
		send2server(s);
		pop_rcv_ok("login-user",1,0);
		sprintf(s,"pass %s",options.pass);
		send2server(s);
		pop_rcv_ok("login-password",1,0);
	} else {
		sprintf(s,"login %s %s",options.user,options.pass);
		send2server(s);
		rcv_ok("login",1);
	}
#if 0
	send2server("lsub \"\" *");
	rcv_ok("lsub",1);
	imap_buf_make_ready();
	for(s[0]=1;s[0];)
	{
		imap_buf_get(s,sizeof(s));
		if ((sp=strstr(s,") ")))
		{
			sp+=2;
			for (i=0;*sp!=0 && (*sp!=' ' || i);sp++) if (*sp=='"') i=i^1;
			if (*sp) sp++;
			if (index(sp,'\n')) *index(sp,'\n')=0;
			if (*sp=='"')
			{
				sp++;
				if (index(sp,'"')) *index(sp,'"')=0;
			}
			if (debug>1) {
				if (*sp) printf("DEBUG: folder: \"%s\"\n",sp);
			}
		}
	}
#endif
	// now read prefix/.onsync and do what it says
	// file syntax is: {delete|seen|unseen} <uid> "<foldername>"
	// for pop3, only delete is valid
	// we currently ignore what we do not understand
	if (options.pop3)
	{
		if (verbose) printf ("getting msg list\n");
		safe_strcpy(s,"uidl");
		send2server(s);
		pop_rcv_ok(s,1,1);
		sp=strstr(imap_buf,"\n");
		for(i=0;sp;)
		{
			j=msg_ids[i][0]=0;
			s[0]=0;
			sscanf(sp+1,"%d %s\n",&msgs[i],s);
			if (strstr(s,"\n")) *strstr(s,"\n")=0;
			/*
			lowstr(s);
			for (j=0;j<strlen(s)&&is_hexchar(s[j]);j++);
			if (j==strlen(s) && j==16 ) sscanf(s+8,"%x",&msg_ids[i]);
			*/
			strncpy(msg_ids[i],s,31);
			msg_ids[i][31]=0;
			sp=index(sp+1,'\n');
			if (msgs[i]&& msg_ids[i][0])
			{
				if (debug>1) {
					printf("DEBUG: msg_ids[%d]=%s\n",msgs[i],msg_ids[i]);
				}
				i++;
			} else sp=0;
		}
		num_msgs=i;
	}
	sprintf(s,"%s/.onsync",options.prefix);
	fl=fopen(s,"r");
	folder[0]=0;
	if (fl)
	{
		if (verbose) printf ("updating server\n");
	}
	if (fl) do
	{
		s[0]=0;
		fgets(s,sizeof(s)-2,fl);
		if (s[0])
		{
			lowstr(s);
			// delete for pop/imap, seen/unseen for imap only
			if ((strncmp(s,"delete ",7)==0||strncmp(s,"seen ",5)==0||strncmp(s,"unseen ",7)==0)&&(s[0]=='d' || ! options.pop3))
			{
				action=s[0];
				sp=s+(action=='s' ? 5:7);
				s2[0]=0;
				sscanf(sp,"%s ",s2);
				i=s2[0];
				if (s2[0])
				{
					sp=index(sp,'"');
					if (sp) 
					{
						sp++;
						if (!index(sp,'"')) sp=0;
					}
					if (sp) 
					{
						*index(sp,'"')=0;
						if (debug>1) printf(" remove uid=%s, folder='%s'\n",s2,sp);
						if (options.pop3)
						{
							for(j=0;s2[0]&&j<num_msgs;j++)
								if (0==strcmp(msg_ids[j],s2))
								{
									sprintf(s,"dele %d",msgs[j]);
									send2server(s);
									if (!pop_rcv_ok(s,0,0)) if (debug) printf("error deleteing msg#%d uid=%d\n",msgs[j],i);
									for(i=j;i<num_msgs;i++) {msgs[i]=msgs[i+1];safe_strcpy(msg_ids[i],msg_ids[i+1]);};
									num_msgs--;
									i=0;
								}
							if (s2[0]&&debug) printf("couldn't find msg with uid=%s",s2);
						} else { // imap
							if (strcmp(sp,folder)!=0)
							{
								if (folder[0])
								{
									send2server("expunge");
									if (!rcv_ok("expunge",0)) if (debug) printf("error on expunge\n");
									send2server("close");
									rcv_ok("close",1);
								}
								safe_strcpy(folder,sp);
								if (debug) printf("opening folder '%s'\n",sp);
								sprintf(s,"select %s",sp);
								send2server(s);
								if (!rcv_ok(s,0)) s2[0]=0;
								if (!s2[0]) if (debug) printf("error on %s \n",s);
							}
							if (s2[0])
							{
								 
								sprintf(s,"search uid %s",s2);
								send2server(s);
								if (!rcv_ok(s,0)) s2[0]=0;
								
								if (!s2[0]) {
									if (debug) printf("error on %s \n",s);
									folder[0]=0;
								} else {
									sp=strstr(imap_buf,"SEARCH ");
									if (sp)
									{
										sp+=strlen("SEARCH ");
										i=0;
										sscanf(sp,"%d",&i);
										if (i)
										{
											if (action!='u') sprintf(s,"store %d +FLAGS (\\%s)",i,action=='d' ? "Deleted":"Seen");
											else sprintf(s,"store %d -FLAGS (\\Seen)",i);

											send2server(s);
											if (!rcv_ok(s,0)) if (debug) printf("error on %s\n",s);
										}
										else if (debug) printf("unknown number : %s\n",sp);
									} else if (debug) printf("'SEARCH' expected\n");
								}
							}
						}
					} else {
						if (debug) printf("couldn't find a '%c' : %s \n",'"',s);
					}
				} else {
					if (debug) printf("unknown number : %s",sp);
				}
					
			} else {
				if (debug) printf("line unrecognized: %s",s);
			}
		}
	} while (!feof(fl));
	if (fl)
	{
		fclose(fl);
		sprintf(s,"/bin/rm -f %s/.onsync",options.prefix);
		system(s);
	}
	if (!options.pop3 && folder[0])
	{
		send2server("expunge");
		if (!rcv_ok("expunge",0)) if (debug) printf("error on expunge\n");
		send2server("close");
		rcv_ok("close",1);
	}
	folder[0]=0;
	
	safe_strcpy(folder,"inbox");
	//safe_strcpy(folder,"jojo");
	if (!options.pop3)
	{
		if (verbose) printf("Opening folder '%s'\n",folder);
		sprintf(s,"select %s",folder);
		send2server(s);
		rcv_ok("select",1);
		//send2server("search not before 1-mar-2002 not larger 5000");
		sprintf(s,"search all");
		if (options.since[0])
		{
			sprintf(s+strlen(s)," not before %s",options.since);
			if (debug>1)
			{
				printf("DEBUG: not before %s\n",options.since);
			}
		}
		/*
		if (options.size)
		{
			sprintf(s+strlen(s)," not larger %d", options.size);
			if (debug>1)
			{
				printf("DEBUG: not larger %d\n",options.size);
			}
		}
		*/
		if (debug>1)
		{
			printf("DEBUG: s=\"%s\"\n",s);
		}
		if (verbose) printf("Getting msg list\n");
		send2server(s);
		rcv_ok("search",1);
		sp=strstr(imap_buf,"SEARCH ");
		if (sp) sp+=strlen("SEARCH");
		for(i=0;sp;)
		{
		 	sscanf(sp+1,"%d",&msgs[i]);
			if (debug>1) {
				//printf("DEBUG: sp=%s",sp+1);
			}
			sp=index(sp+1,' ');
			if (msgs[i])
			{
				if (debug>1) {
					printf("DEBUG: msg[%d]=%d\n",i,msgs[i]);
				}
				i++;
			} else sp=0;
		}
		num_msgs=i;
		if (debug>1) {
			printf("DEBUG: total of %d msgs\n",num_msgs);
		}
	}
	if (verbose) printf("Getting messages\n");
	for(i=0,flt=0;i<num_msgs;i++)
	{
		if (verbose>1)
		{
			// 50 '#' chars for whole msgs
			while((flt+1)<((i+1)*(50.0/(num_msgs))))
			{
				flt++;
				printf("#");
				fflush(stdout);
			}
		}
		if (options.pop3)
		{
		  sprintf(s,"list %d",msgs[i]);
		} else {
		  sprintf(s,"fetch %d (RFC822.SIZE)",msgs[i]);
		}
		send2server(s);
		if (options.pop3)
		{
			pop_rcv_ok("fetch size",1,0);
			sp=strstr(imap_buf,"+OK ");
			if (sp) sp=strstr(sp+4," ");
		} else {
			rcv_ok("fetch size",1);
			sp=strstr(imap_buf,"SIZE ");
		}
		if (!sp) error ("fetch","can't get size");
		sp=strstr(sp," ");
		sscanf(sp,"%d",&size);
		if (!options.pop3)
		{
			sprintf(s,"fetch %d (UID)",msgs[i]);
			send2server(s);
			rcv_ok("fetch uid",1);
			sp=strstr(imap_buf,"UID ");
			msg_ids[i][0]=0;
		}
		if (sp)
		{
			if (!options.pop3)
			{
				sp+=4;
				sscanf(sp,"%s",msg_ids[i]);
				if (index(msg_ids[i],')')) *index(msg_ids[i],')')=0;
			}
			if (index(sp,')')||options.pop3)
			{
				//*index(sp,')')=0;
				sprintf(s,"%s/%s/%s.head",options.prefix,folder,msg_ids[i]);
				if (debug>1) {
					printf("DEBUG: filename: \"%s\"\n",s);
				}
				sprintf(s2,"%s/%s/%s.body",options.prefix,folder,msg_ids[i]);

				if (!size) fprintf(stderr,"Warning: error reading size\n");
				get_body=size&&(size<=options.size);
				get_body=get_body||!options.size;
				if (!file_exists(s)||(!file_exists(s2)&&get_body))
				{
					if (debug) printf("downloading msg %d uid=%s\n",msgs[i],msg_ids[i]);
					if (!options.pop3)
					{
						fl=openfile(s,0);
						sprintf(s,"fetch %d (BODY.PEEK[HEADER])",msgs[i]);
						send2server(s);
						sprintf(s,"fetch header of msg[%d]",msgs[i]);
						rcv_ok(s,1);
						sp=index(imap_buf,'\n');
						if (sp)
						{
							sp2=index(sp+strlen(sp)-4,')');
							if (sp2) *sp2=0;
							fprintf(fl,"%s",sp+1);
						} else error ("","head missing");
						fclose(fl);
					} else { // pop3
						if (get_body) fl=openfile(s,0);
						/*
						if (!get_body)
						{
							fprintf(fl,"From: message too big, not downloaded\n"
										  "To:\n"
										  "Subject:\n"
										  "Date:\n"
							fclose(fl);
						}*/
					}
					if (get_body)
					{
						if (options.pop3)
						{
							sprintf(s,"retr %d",msgs[i]);
							send2server(s);
							pop_rcv_ok(s,1,1);
							sp=index(imap_buf,'\n');
							if (sp) sp++;
							while (sp && *sp!='\n')
							{
									  if (!strstr(sp,"\n")) sp=0;
									  if (sp)
									  {
											i=strstr(sp,"\n")-sp;
											sp[i]=0;
											fprintf(fl,"%s\n",sp);
											sp+=i+1;
									  }
							}
							fclose(fl);
						} else { // imap
							sprintf(s,"fetch %d (BODY.PEEK[TEXT])",msgs[i]);
							send2server(s);
							sprintf(s,"fetch body of msg[%d]",msgs[i]);
							rcv_ok(s,1);
							sp=index(imap_buf,'\n');
							if (sp)
							{
								sp2=index(sp+strlen(sp)-4,')');
								if (sp2) *sp2=0;
							}
						}
						if (sp)
						{
							fl=openfile(s2,0);
							fprintf(fl,"%s",sp+1);
							fclose(fl);
						} else error (s,"body missing");


					} else if (debug) printf("msg too big (%d>%d), not writing %s\n",size,options.size,s2);
				} else {
					if ((debug>1)&&size) {

						if (get_body)
							printf("both %s and it's body exists, not ovveriding\n",s);
						else
							printf("%s exists, not ovveriding\n",s);
					}
				}
				if (file_exists(s2)&&!get_body)
				{
					fprintf(stderr,"Warning: %s exists, but msg too big, %sremoving\n",s2, options.remove ? "":"not ");
					if (options.remove) if (unlink(s2)) perror("unlink");
				}
				if (get_body && ! options.pop3 )
				{
					if (debug) printf("getting flags\n");
					sprintf(s,"fetch %d FLAGS",msgs[i]);
					send2server(s);
					sprintf(s,"fetch flags of msg[%d]",msgs[i]);
					rcv_ok(s,1);
					sp=index(imap_buf,'(');
					if (sp) sp=index(sp+1,'(');
					if (sp)
					{
						sp++;
						sp2=index(sp+strlen(sp)-4,')');
						if (sp2) *sp2=0;
					}
					if (sp)
					{
						if (debug>2) printf("flags: %s\n",sp);
						siof=sizeof(s2);
						s2[siof-1]=0;
						strncpy(s2+strlen(s2)-4,"flags",siof-strlen(s2)+3);
						fl=openfile(s2,0);
						siof=sizeof(s2);
						s2[siof-1]=0;
						strncpy(s2+strlen(s2)-5,"body",siof+4-strlen(s2));
						fprintf(fl,"%s",sp);
						fclose(fl);
					} else error (s,"flags missing");
				}
			}
		}
	}
	if (verbose>1) printf("\n");
	if (options.pop3)
	{
		safe_strcpy(s,"quit");
		send2server(s);
		if (!pop_rcv_ok(s,0,0)) printf("Warning: could not quit server, msgs will not be deleted\n");
	}
	if (options.cleandir)
	{
		if (verbose) printf("cleaning old msgs\n");
		sprintf(s,"%s/%s",options.prefix,folder);
		dir=opendir(s);
		if (!dir) {perror(s);exit(1);}
		do
		{
			direntptr=readdir(dir);
			if (direntptr)
			{
				for(i=flag=0;i<num_msgs&&!flag;i++)
				{
					sprintf(s2,"%s.head",msg_ids[i]);
					if (strcmp(s2,direntptr->d_name)==0) flag=1;
					else {
						sprintf(s2,"%s.body",msg_ids[i]);
						if (strcmp(s2,direntptr->d_name)==0) flag=1;
						else {
							sprintf(s2,"%s.flags",msg_ids[i]);
							if (strcmp(s2,direntptr->d_name)==0) flag=1;
						}
					}
				}
				if (debug>1) printf("file %s %sfound\n",direntptr->d_name, flag ? "":"not ");
				if (!flag)
				{
					sprintf(s2,"%s/%s",s,direntptr->d_name);
					if (debug>1) printf("removing %s\n",s2);
					if (unlink(s2)) if (errno!=EISDIR) perror("unlink");
				}

			}
		} while (direntptr);
	}
#if 0
	for(;;)
	{
		// copy child to stdout
		i=read(fd_in,s,sizeof(s));
		if (i) {write (1,s,i);fsync(1);}
		// copy stdin to child
		// i=read(0,s,sizeof(s));
		// if (i) {write (fd_out,s,i);fsync(fd_out);}
		// sleep some time
		usleep(1000); // 1000 us (1ms)
	}
#endif
	return 0;
}

