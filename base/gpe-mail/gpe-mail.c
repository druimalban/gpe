/*
 * gpe-mail
 *
 * by Erez Doron <erez@savan.com>
 */

#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include "gtkcolombo.h"
#include <stdlib.h>
#include <stdio.h>

#define VERSION "0.0.8-gtk2"

#define gtk_text_thaw(a) 
#define Gtk_entry_set_text(a,b) gtk_entry_set_text(a,g_locale_to_utf8(b,-1,NULL,NULL,NULL))

#define CONF_YES	"\1"
#define CONF_NO		"\2"
struct {
   char *path;
   char expanded_path[100];
   char conf[3][10][100];
   int num_new;
} config={
   "~/.emailsync",
   "",
   {
      {
         "root","secret","localhost","auto",CONF_NO,CONF_YES,"5000","7","30000",""
      },
      {
         "root@localhost","smtp","auto",CONF_NO,"","","","","",""
      },
      {
         CONF_YES,CONF_NO,"/bin/sqlite","","","","","","",""
      }
   }
};

struct {
   gchar *titles[4];
   gchar *opts[3][10];
} options={
   {"Incoming","Outgoing","Misc",NULL},
   {
      {"Username","Password","Server","Port","Use pop3 instead of imap",
       "SSL","Size (bytes)","Since (days), imap only ","Server Timeout in ms",NULL},
      {"Email Adress","Smtp Server","Port","SSL",NULL,NULL,NULL,NULL,
       NULL,NULL},
      {"Mark msgs as seen when viewed",
       "Open Alternative text/html as html in dillo",
       "Path to Sqlite",NULL,NULL,NULL,NULL,NULL,NULL,NULL}
   }
};

struct msg_body_t {
    GtkWidget *win;
    GtkWidget *vb;
    GtkWidget *hb;
    GtkWidget *sw;
    // text
    GtkWidget *t;
    GtkTextBuffer *tb;
    // buttons
    GtkWidget *deleteb;
    GtkWidget *forwardb;
    GtkWidget *replyb;
    GtkWidget *closeb;
};

struct compose_t {
    GtkWidget *win;
    GtkWidget *vb1;
    GtkWidget *hb;
    GtkWidget *t;
    GtkWidget *text;
    GtkTextBuffer *text_buffer;
    GtkWidget *sw;
    GtkWidget *entries[5];
};

struct {
   GtkWidget *w;
   GtkWidget *b0;
   GtkWidget *hb2;
   GtkWidget *sw2;
   //GtkWidget *sw3;
   struct msg_body_t body;
   GtkWidget *vb3;
   GtkWidget *b2;
   GtkWidget *folderoption;
   GtkWidget *foldermenu;
   GtkWidget *cl1;
   //GtkWidget *mt;
   //GtkTextBuffer *mtb;
   GtkTooltips *tt;

   GtkWidget *configb;
   GtkWidget *syncb;
   GtkWidget *composeb;


   struct {
       GtkWidget *win;
       GtkWidget *nb;
       GtkWidget *fr[3];
       GtkWidget *fr_lbl[3];
       GtkWidget *box[4];
       GtkWidget *hb[3][10];
       GtkWidget *lbl[3][10];
       GtkWidget *e[3][10];
       GtkWidget *l[3][10];
   }conf;

   struct {
      FILE *f;
      char part;
      GtkWidget *win;
      GtkWidget *sw;
      //GtkWidget *box1;
      GtkWidget *t; // GtkTextView
      GtkTextBuffer *tb; // GtkTextBuffer
   } sync;

   char cur_msg[32];
   int cl1_row;
   int sigchild;
   char sortby_last;
   char current_folder[100];
   char fromto[10];
} self;

GtkTextIter start_iter, end_iter;

struct dirent * direntptr;
DIR *dir;
FILE *f;
char encoding_name[30];

void my_locale_to_utf8(char *s)
{
	char *s2;
	if (!s[0]) return;
	if (encoding_name[0]==0)
	{
	   s2=g_locale_to_utf8(s,-1,NULL,NULL,NULL);
	}
	else
	{
	   s2=g_convert (s, -1, "UTF-8", encoding_name,NULL,NULL, NULL);
	   //printf(" '%s' (%s) converted to '%s' (utf8)\n",s,encoding_name,s2);
	}
	if (s2==NULL)
	{
	   s[0]=0;
	   fprintf(stderr,"Warning: can't convert '%s' from '%s' to 'utf8'\n",
	   		s,encoding_name[0] ? encoding_name:"local-encoding");
	   return;
	}
	if (s2[0]==0)
	   fprintf(stderr,"Warning: '%s' ('%s') converted to en empty string\n",
	   	s,encoding_name);
	strcpy(s,s2);
}

inline char file_exists(char *s)
{
   struct stat st;
   char c;
   c=(!stat(s,&st));
   return c;
}

inline char starts_with(char *str, char *substr)
{
        return (strstr(str,substr)==str);
}

inline char ends_with(char *str, char *substr)
{
        return (strcmp(str+strlen(str)-strlen(substr),substr)==0);
}


char *b2a_base64(char *s)
{
   const char base64set[64]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   static char *s2=NULL;
   int i,j,val,ofs;

   if (s2!=NULL) free(s2);
   s2=(char *)malloc(strlen(s)*8/6+10);
   for(i=j=val=ofs=0;s[i];i++)
   {
      s2[j++]=base64set[((val<<(6-ofs))|(s[i]>>(ofs+2)))];
      val=((1<<(ofs+2))-1)&s[i];
      ofs=(ofs+8)%6;
      if (!ofs)
      {
         s2[j++]=base64set[val];
         val=0;
      }

   }
   if (ofs)
   {
      s2[j++]=base64set[((val<<(6-ofs))|(s[i]>>(ofs+2)))];
      for(ofs=(6-ofs)/2;ofs;ofs--) s2[j++]='=';
   }
   s2[j++]=0;
   return s2;
}

char *get_config(char *section,char *param);

FILE *search_email_open_file(char *key, char *database_path)
{
	int i=0;
	char s[1000];
	char sqlite[100];
	FILE *f;

	strncpy(sqlite,get_config("Misc","Path to Sqlite"),sizeof(sqlite));
	if (!*key) return NULL;
	if (!file_exists(sqlite))
	{
		if (!i) printf("sqlite not found at %s\n",sqlite);
		i=1;
		return NULL;
	}
	//sprintf(s,"'echo \"select * from contacts where tag like '\"'\"'%%EMAIL'\"'\"' and urn = ( select urn from contacts where (tag='\"'\"'NAME'\"'\"' and value like '\"'\"'%s%%'\"'\"') or ( tag like '\"'\"'%%EMAIL'\"'\"' and value like '\"'\"'%s%%'\"'\"'));\" |/usr/bin/sqlite %s|sed '\"'\"'s/|.*//'\"'\"' '",key,key,database_path);
	//printf("key='%s'\n",key);
	sprintf(s,"echo \"select * from contacts where tag like '%%EMAIL' and urn = ( select urn from contacts where (tag='NAME' and value like '%s%%') or ( tag like '%%EMAIL' and value like '%s%%'));\" |%s %s|sed 's/.*|//' ",key,key,sqlite,database_path);
	//printf("exeuting: %s\n",s);
	f=(FILE *)popen(s,"r");
	//printf("executed\n");
	return f;
}

int tilde(char *file,char * exp);
char *search_email(char *key)
{
	char *sp;
	static FILE *f;
	static char s[200];
	static char *saved_key;

	if (key!=NULL)
	{
		saved_key=key;
		tilde("~/.gpe/contacts",s);
		f=search_email_open_file(saved_key,s);
	}
	sp=NULL;
	if (f!=NULL)
	{
		sp=(char *)fgets(s,sizeof(s)-1,f);
		if (NULL==sp)
		{
			pclose(f);
			f=NULL;
		} else {
			if (*(sp+strlen(sp)-1)=='\n') *(sp+strlen(sp)-1)=0;
		}
	}
	//if (sp) printf(" -> %s\n",sp);
	return sp;
	/*
	while (f!=NULL)
	{
		sp=fgets(s,sizeof(s)-1,f);
		if (sp!=NULL)
			printf(" -> %s",sp);
		else
		{
			pclose(f);
			f=NULL;
		}
	}
	*/

}



void a2b_base64(char *s)
{
   signed char c;
   int i,j,val,ofs;
   static char initialized=0;
   static signed char conv[256];

   //printf("a2b_base64: input(len=%d):\n%s\n",strlen(s),s);
   if (!initialized)
   {
      for(i=0;i<256;i++)
      {
      if (i<='Z' && i>='A' ) conv[i]=i-'A';
      else
           {
         if (i<='z' && i>='a' ) conv[i]=i-'a'+26;
         else
         {
            if (i<='9' && i>='0' ) conv[i]=i-'0'+52;
       else if (i=='+') conv[i]=62;
       else if (i=='/') conv[i]=63;
       else conv[i]=-1;
         }
      }
      }
      initialized=1;
   }

   for(i=j=val=ofs=0;s[j];j++)
   {
      c=conv[(int)s[j]];
      if (c>=0)
      {
         if (!ofs)
         {
         ofs=6;
         val=c;
         } else {
            s[i++]=(val<<(8-ofs))|(c>>(ofs-2));
            ofs=ofs-2;
            val=c&((1<<ofs)-1);
         }
      }
   }
   s[i++]=0;
}


static int get_timezone ( void )           /* compute offset from GMT */
{
#define FDAY (24*60*60)                    /* full day in seconds */
#define HDAY (FDAY/2)                      /* half day in seconds */
struct tm ofst;
long  tloc ;                               /* seconds from 1/1/1970 0.0.0 GMT */
int sec_ofst ;

   time (& tloc) ;                         /* get current time */
   ofst = *gmtime (& tloc) ;               /* find GMT hour */
   sec_ofst = ( ofst.tm_hour * 60 +
                ofst.tm_min ) * 60 +
                ofst.tm_sec ;              /* GMT hour (in seconds) */
   ofst = *localtime (& tloc) ;               /* find GMT hour */
   sec_ofst = ( ( ofst.tm_hour * 60 +
                  ofst.tm_min ) * 60 +
                  ofst.tm_sec - sec_ofst
                + FDAY ) % FDAY ;          /* Local - GMT hours (in seconds, 0-FDAY) */
   if ( sec_ofst > HDAY )
       sec_ofst -= FDAY ;                  /* sec_ofst in range -11:59:59 +12:00:00 */
   return ( sec_ofst ) ;
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
          strcpy(user, login);
      }
      if (pw == NULL && (pw = getpwnam(user)) == NULL)
        return (0);
      strcpy(exp, pw->pw_dir);
    }
    strcat(exp, file);
    return (1);
  } return (0);
}

void trunc_newlines(char *s)
{
   while (s[0] && ((s[strlen(s)-1]=='\n')||(s[strlen(s)-1]=='\r'))) s[strlen(s)-1]=0;
}

int hexchar_sub(char c,char w)
{
   if (c>='0' && c<='9') return c-'0';
   if (c>='A' && c<='F') return c-'A'+10;
   if (c>='a' && c<='f') return c-'a'+10;
   if (w) printf("error, char(%d) is not a hexchar\n",(int)c);
   return 16;

}

inline int hexchar(char c){return hexchar_sub(c,1);}
inline char is_hexchar(char c){return hexchar_sub(c,0)!=16;}

void q2b(char *s)
{
   char *sp;

   //printf("q2b: before: %s\n",s);
   sp=s;
   while (*sp)
   {
      //printf("s=%s\n",s);
      while (*sp&&(*sp!='=')&&(*sp!='_')) sp++;
      if (*sp=='_') *sp++=' ';
      else if (*sp=='='&&is_hexchar(*(sp+1))&&is_hexchar(*(sp+2)))
      {
         *(sp)=16*hexchar(*(sp+1))+hexchar(*(sp+2));
	 sp++;
         strcpy(sp,sp+2);
      } else sp++;
      //printf("num=%u",(unsigned int)(*(sp-1)));
      //printf(" = 0x%X",(unsigned int)(*(sp-1)));
      //printf(" = '%c'\n",(unsigned char)(*(sp-1)));
      
   }
   //printf("q2b: after: %s\n",s);
}

void convert(char *s)
{
   char *sp;
   char *sp2;
   char method;

   //printf("\nconvert: got '%s', ",s);
   
   // skip spaces
   sp=s;
   while (*sp==' ') sp++;
   if (s!=sp) strcpy(s,sp);

   encoding_name[0]=0;
   // look for '=?', if not found do return
   if (NULL==(sp=strstr(s,"=?"))) return;
   //if (starts_with(sp,"=?windows-1255?")) heb=1;


   sp2=sp;
   sp+=2; // skip the =?
   while (*sp && *sp!='?')sp++; // skip the encoding
   *sp=0;
   strncpy(encoding_name,sp2+2,sizeof(encoding_name));
   sp++; // skip the ?
   method=*(sp++); // find if Q or B
   sp++; // skip the ?
   if (method=='B') a2b_base64(sp);
   else if (method=='Q') q2b(sp);
   if (strstr(sp,"?=")!=NULL) *strstr(sp,"?=")=0; // trancate '?=' till end
   strcpy(sp2,sp);

   //printf("convert: returning '%s'\n",s);
   
}

char *get_config(char *section,char *param)
{
   int i,j;

   for(j=0;options.titles[j];j++)
   {
      if (0==strcmp((options.titles[j]),(section)))
      {
         for(i=0;options.opts[j][i];i++)
         {
            if (0==strcmp((options.opts[j][i]),(param)))
            return config.conf[j][i];
      }
      printf("param '%s' not found\n",param);
      return NULL;
      }
   }
   printf("section '%s' not found\n",section);
   return NULL;
}

char get_config_boolean(char *section,char *param)
{
	char c;
	
	c=*get_config(section,param);
	return 2-c;
}

GtkWidget *mkbutton(char *xpm_name, char *but_name,char *text)
{
   const char xpm_dirs[]="/usr/share/gpe/pixmaps/default|../gpe-icons/default|/usr/share/gpe-mail/icons|./xpms|/home/erez/xpms";
   const char exts[]="png|xpm";
   char xpm_path[100];
   char ext[100];
   int i,j,k,l;
   GtkWidget *image;
   GtkWidget *b;

   if (!*text) text=but_name;
   j=0;
   while (xpm_dirs[j]&&j>=0)
   {
      for (i=j;xpm_dirs[i]&& xpm_dirs[i]!='|';i++) xpm_path[i-j]=xpm_dirs[i];
      xpm_path[(i++)-j]='/';
      xpm_path[i-j]=0;
      j=i;
      strcat(xpm_path,xpm_name);
      l=strlen(xpm_path);
      xpm_path[l++]='.';
      k=0;
      while (exts[k]&&k>=0&&j>=0)
      {
         for (i=k;exts[i]&&exts[i]!='|';i++) ext[i-k]=exts[i];
         ext[i-k]=0;
	 if (k!=i) k=i;
	 else k++;
         xpm_path[l]=0;
         strcat(xpm_path,ext);
	 //printf("trying '%s' ... ",xpm_path);
         if (file_exists(xpm_path)) j=-1;
	 //if (j<0) printf("found\n");
	 //else printf("not found\n");
      }
   }
   if (j>=0) return gtk_button_new_with_label(but_name);
   //printf("using icon file: %s\n",xpm_path);
   b=gtk_button_new();
   image=gtk_image_new_from_file (xpm_path);
   gtk_container_add (GTK_CONTAINER (b), image);
   gtk_widget_show(image);
   gtk_tooltips_set_tip(self.tt,b,text,NULL);
   return b;
   
}

void loadconfig()
{
   FILE *f;
   char s[100],s2[100];
   char *sp,*sp2;
   int i,j;

   tilde("~/.gpemailrc",s);
   f=(FILE *)fopen(s,"r");
   if (!f)
   {
      perror("fopen:");
      return;
   }
   s[0]=1;
   while (s[0])
   {
      s[0]=0;
      fgets(s,sizeof(s),f);
      if (s[0])
      {
         for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
         {
            sprintf(s2,"\"%s.%s\"",options.titles[j],options.opts[j][i]);
            if (NULL!=(sp=strstr(s,s2)))
            {
               sp2=strchr(s,'#');
               if ((sp2==NULL)||(sp2>sp))
               {
                  sp+=strlen(s2);
                  sp=strchr(sp,'=');
                  if (sp!=NULL)
                  {
                     sp++;
                     while (*sp==' '||*sp=='\t') sp++;
                     if (*sp=='"')
                     {
                        sp++;
                        sp2=strrchr(sp,'"');
                        if (sp2!=NULL)
                        {
                           *sp2=0;
                           strcpy(config.conf[j][i],sp);
                        }
                     } else {
                        if (*sp=='0'||*sp=='1')
                        {
                           if (*sp=='0') config.conf[j][i][0]=2;
                           else
                              config.conf[j][i][0]=1;
                           config.conf[j][i][1]=0;
                        }

                     }
                  }
               }
            }

         }
      }
   }
   for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
   {
      if (config.conf[j][i][0]&&(config.conf[j][i][0]<3))
         fprintf(f,"\"%s.%s\"=%d\n",options.titles[j],options.opts[j][i],(int)config.conf[j][i][0]%2);
      else
         fprintf(f,"\"%s.%s\"=\"%s\"\n",options.titles[j],options.opts[j][i],config.conf[j][i]);

   }
   fclose(f);
}

void saveconfig()
{
   FILE *f;
   char s[100];
   int i,j;

   tilde("~/.gpemailrc",s);
   f=(FILE *)fopen(s,"w");
   if (!f)
   {
      perror("fopen:");
      return;
   }
   for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
   {
      if (config.conf[j][i][0]&&(config.conf[j][i][0]<3))
         fprintf(f,"\"%s.%s\"=%d\n",options.titles[j],options.opts[j][i],(int)config.conf[j][i][0]%2);
      else
         fprintf(f,"\"%s.%s\"=\"%s\"\n",options.titles[j],options.opts[j][i],config.conf[j][i]);

   }
   fclose(f);
}

void configbutton_cancel(GtkButton *button, gpointer user_data)
{
   gtk_widget_hide(self.conf.win);
   gtk_widget_destroy(self.conf.win);
   self.conf.win=NULL;
}

void configbutton_ok(GtkButton *button, gpointer user_data)
{
   int i,j,k;

   for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
   {
      if (config.conf[j][i][0]&&(config.conf[j][i][0]<3))
      {
         k=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.conf.l[j][i]));
         if (!k) k=2;
         config.conf[j][i][0]=k;
         config.conf[j][i][1]=0;
      } else {
	 if ((0!=strcmp(options.opts[j][i],"Password"))||(0!=strcmp("***",gtk_entry_get_text(GTK_ENTRY(self.conf.e[j][i])))))
         strcpy(config.conf[j][i],gtk_entry_get_text(GTK_ENTRY(self.conf.e[j][i])));
      }

   }
   gtk_widget_hide(self.conf.win);
   saveconfig();
   tilde(config.path,config.expanded_path);
   strcat(config.expanded_path,(1-((*get_config("Incoming","Use pop3 instead of imap"))-1)) ? "/pop":"/imap");
   gtk_widget_destroy(self.conf.win);
   self.conf.win=NULL;
}

void do_config(GtkButton *button, gpointer user_data)
{
   int i,j;
   GtkWidget *bx,*bt1,*bt2;

   if ((NULL==self.conf.win)||!GTK_IS_WIDGET(self.conf.win))
   {
      self.conf.win=gtk_window_new(GTK_WINDOW_TOPLEVEL);

      self.conf.nb=gtk_notebook_new();
      gtk_notebook_set_tab_pos(GTK_NOTEBOOK(self.conf.nb),GTK_POS_TOP);


      for (i=0;options.titles[i];i++)
      {
         self.conf.fr[i]=gtk_frame_new(options.titles[i]);
         gtk_container_set_border_width(GTK_CONTAINER(self.conf.fr[i]),10);
         gtk_frame_set_shadow_type(GTK_FRAME(self.conf.fr[i]),GTK_SHADOW_ETCHED_OUT);

         self.conf.fr_lbl[i]=gtk_label_new(options.titles[i]);
         gtk_notebook_append_page(GTK_NOTEBOOK(self.conf.nb),self.conf.fr[i],self.conf.fr_lbl[i]);
      }

      for(i=0;i<4;i++)
      {
         self.conf.box[i]=gtk_vbox_new(FALSE,0);
         if (i<3) gtk_container_add(GTK_CONTAINER(self.conf.fr[i]),self.conf.box[i]);
      }
        gtk_box_pack_start (GTK_BOX (self.conf.box[3]), self.conf.nb, TRUE, TRUE, 0);
      gtk_container_add(GTK_CONTAINER(self.conf.win),self.conf.box[3]);

      for(j=0;options.titles[j];j++) for(i=0;options.opts[j][i];i++)
      {
         self.conf.hb[j][i]=gtk_hbox_new(FALSE,0);
           gtk_box_pack_start (GTK_BOX (self.conf.box[j]), self.conf.hb[j][i], TRUE, TRUE, 0);
         if (config.conf[j][i][0]&&(config.conf[j][i][0]<3))
         {
            self.conf.l[j][i]=gtk_check_button_new_with_label(options.opts[j][i]);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.conf.l[j][i]),config.conf[j][i][0]&1);
         }
         else
         {
         
            self.conf.l[j][i]=gtk_label_new(options.opts[j][i]);
            self.conf.e[j][i]=gtk_entry_new();
              gtk_box_pack_end (GTK_BOX (self.conf.hb[j][i]), self.conf.e[j][i], TRUE, TRUE, 0);
            if (j==0&&(0==strcmp(options.opts[j][i],"Password")))
               Gtk_entry_set_text(GTK_ENTRY(self.conf.e[j][i]),"***");
            else
               Gtk_entry_set_text(GTK_ENTRY(self.conf.e[j][i]),config.conf[j][i]);
         }
           gtk_box_pack_start (GTK_BOX (self.conf.hb[j][i]), self.conf.l[j][i], TRUE, TRUE, 0);
      }
      bx=gtk_hbox_new(FALSE,0);
      gtk_box_pack_start (GTK_BOX (self.conf.box[3]), bx, FALSE, FALSE, 0);
      bt1=mkbutton("cancel","Cancel","");
      bt2=mkbutton("ok","OK","");
      gtk_box_pack_start (GTK_BOX (bx), bt1, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (bx), bt2, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT(bt1), "clicked", 
         (GtkSignalFunc) configbutton_cancel, NULL);
      gtk_signal_connect (GTK_OBJECT(bt2), "clicked", 
         (GtkSignalFunc) configbutton_ok, NULL);


   }
   gtk_widget_show_all(self.conf.win);
}

int hide_sync_win()
{
   usleep(100000);
   if (self.sync.part++<22) return 1;
   gtk_widget_hide(self.sync.win);
   return 0;
}

int do_sync2();
int open_mailfolder();

int update_sync_text()
{
   char s[100];
   int i;
   
   //printf("before usleep\n");
   usleep(100000);
   i=fread(s,1,sizeof(s)-1,self.sync.f);
   if (i<1) {
      if (!self.sigchild) return 1;
      i=pclose(self.sync.f);
      if (!i&&self.sync.part==1)
      {
         gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)self.sync.tb,"\n\n\nReceive:\n\n",-1);
         gtk_idle_add(do_sync2,NULL);
      }
      else
      {
         if (!i) gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)self.sync.tb,"\n\nDone\n",-1);
	 else gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)self.sync.tb,"\n\nError occured !\n",-1);
         gtk_idle_add(hide_sync_win,NULL);
         gtk_idle_add(open_mailfolder,NULL);
      }
      self.sync.part=2;
      return 0;
   }
   s[i]=0;
   my_locale_to_utf8(s);
   gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)self.sync.tb,s,-1);
   return 1;
}

void onsigchild(int i)
{
   //printf("sigchild ...\n");
   self.sigchild=1;
}

int do_sync2()
{
   char s[200];
   char s2[5],s3[5];
   int dy,hh,mm,ss,yy;
   time_t t;
   int i;

   sprintf(s,"/usr/bin/emailsync server=%s port=%s ssl=%d user=%s pass=%s proto=%s verbose=2 prefix=%s",get_config("Incoming","Server"),get_config("Incoming","Port"),(int)(1-((*get_config("Incoming","SSL"))-1)),get_config("Incoming","Username"),get_config("Incoming","Password"),(1-((*get_config("Incoming","Use pop3 instead of imap"))-1)) ? "pop":"imap",config.expanded_path);
   if (*get_config("Incoming","Size (bytes)"))
   {
      strcat(s," size=");
      strcat(s,get_config("Incoming","Size (bytes)"));
   }
   if (*get_config("Incoming","Server Timeout in ms"))
   {
      strcat(s," timeout=");
      strcat(s,get_config("Incoming","Server Timeout in ms"));
   }
   if (*get_config("Incoming","Since (days), imap only "))
   {
      i=0;
      sscanf(get_config("Incoming","Since (days), imap only "),"%d",&i);
      if (-1==time(&t)) i=0 ;
      if (i)
      {
         t-=24*3600*i;
         // convert "Mon Jul 29 14:47:20 2002" -> "29-Jul-2002"
         sscanf(ctime(&t),"%s %s %d %d:%d:%d %d",s2,s3,&dy,&hh,&mm,&ss,&yy);
         sprintf(s+strlen(s)," since=%02d-%s-%04d",dy,s3,yy);
      }
   }
   strcat(s," 2>&1 ");
   //printf("cmdline: %s \n",s);
   self.sigchild=0;
   self.sync.f=(FILE *)popen(s,"r");
   if (self.sync.f==NULL) {printf("error\n");gtk_widget_hide(self.sync.win);return 0;}
   fcntl(fileno(self.sync.f),F_SETFL,O_NONBLOCK);
   self.sync.part=2;
   //printf("before idle add #2\n");
   gtk_idle_add(update_sync_text,NULL);
   return 0;
}

void do_sync(GtkButton *button, gpointer user_data)
{
   char s[200];
   char do_send;

   do_send=1;

   sprintf(s,"%s/OUTBOX",config.expanded_path);
   if (!file_exists(s)) do_send=0;
   sprintf(s,"/usr/bin/emailsync server=%s port=%s ssl=%d proto=smtp verbose=2 prefix=%s",get_config("Outgoing","Smtp Server"),get_config("Outgoing","Port"),(int)(1-((*get_config("Outgoing","SSL"))-1)),config.expanded_path);
   strcat(s," 2>&1 ");
   //printf("cmdline(smtp): %s \n",s);
   if ((NULL==self.sync.win)||!GTK_IS_WIDGET(self.sync.win))
   {
      self.sync.win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
      self.sync.sw=gtk_scrolled_window_new(NULL,NULL); 
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self.sync.sw),GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
      gtk_widget_show(self.sync.sw);
      //self.sync.box1=gtk_vbox_new(FALSE,0);
      gtk_container_add(GTK_CONTAINER(self.sync.win),self.sync.sw);
      self.sync.t=gtk_text_view_new();
      self.sync.tb=gtk_text_view_get_buffer (GTK_TEXT_VIEW (self.sync.t));
      gtk_container_add(GTK_CONTAINER(self.sync.sw),self.sync.t);
      //gtk_box_pack_start (GTK_BOX (self.sync.box1), self.sync.t, TRUE, TRUE, 0);
      gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)self.sync.tb,"Sending mail:\n\nemailsync output:\n\nSend:\n",-1);

   }
   gtk_window_set_title(GTK_WINDOW(self.sync.win),"Send-Receive");
   gtk_text_buffer_set_text (self.sync.tb, "", -1);
   gtk_widget_show_all(self.sync.win);
   self.sigchild=0;
   signal(SIGCHLD,onsigchild);
   if (do_send)
   {
      self.sync.f=(FILE *)popen(s,"r");
      if (self.sync.f==NULL) {printf("error\n");gtk_widget_hide(self.sync.win);return;}
      fcntl(fileno(self.sync.f),F_SETFL,O_NONBLOCK);
      self.sync.part=1;
      //printf("before idle add\n");
      gtk_idle_add(update_sync_text,NULL);
      //sprintf(s,"/usr/bin/emailsync server=%s user=%s pass=%s port=%s size=%s proto=%s ssl=%s since=%s verbose=2
   } else do_sync2();
}

void msg_body_destroy(GtkButton *button, gpointer user_data)
{
   struct msg_body_t *body;

   body=(struct msg_body_t *)user_data;
   if (body==&self.body || body==NULL) return;
   gtk_widget_hide(body->win);
   gtk_widget_destroy(body->win);
   free(body);
}

void do_delete(GtkButton *button, gpointer user_data)
{
   char s[200];
   struct msg_body_t *body;

   body=(struct msg_body_t *)user_data;
   if (body==NULL) body=&self.body;

   if (!self.cur_msg[0]) return;
   //printf("delete: %s\n",self.cur_msg[0]);
   sprintf(s,"/bin/rm -f %s/%s/%s.*",config.expanded_path,self.current_folder,self.cur_msg);
   if (0!=strcmp(self.current_folder,"OUTBOX"))
      sprintf(s+strlen(s),";echo 'delete %s \"%s\"' >> %s/.onsync",self.cur_msg,self.current_folder,config.expanded_path);
   //printf("system('%s')\n",s);
   system(s);
   self.cur_msg[0]=0;
   gtk_text_buffer_set_text(body->tb, "", -1);

   gtk_clist_freeze(GTK_CLIST(self.cl1));
   gtk_clist_remove(GTK_CLIST(self.cl1),self.cl1_row);
   if (gtk_clist_row_is_visible(GTK_CLIST(self.cl1),self.cl1_row))
      gtk_clist_select_row(GTK_CLIST(self.cl1),self.cl1_row,1);
   else
   if (gtk_clist_row_is_visible(GTK_CLIST(self.cl1),self.cl1_row-1))
   {
      self.cl1_row--;
      gtk_clist_select_row(GTK_CLIST(self.cl1),self.cl1_row,1);
      
   } else self.cl1_row=-1;
   gtk_clist_thaw(GTK_CLIST(self.cl1));
   msg_body_destroy(NULL,body);
}


void compose_window_destroy(GtkButton *button, struct compose_t *compose)
{
   gtk_widget_hide(compose->win);
   gtk_widget_destroy(compose->win);
   free(compose);
}

void compose_window_update_title(GtkEntry *e, struct compose_t *compose)
{
   char s2[100]="Compose: ";
   char *s=s2+9;

   strcpy(s,gtk_entry_get_text(GTK_ENTRY(compose->entries[4])));
   gtk_window_set_title(GTK_WINDOW(compose->win),s2);

}


int do_folders();

void compose_window_send(GtkButton *button, struct compose_t *compose)
{
   time_t t;
   char s[1000];
   char s2[100];
   char s3[10];
   char *sp;
   int i;
   int h,m,sec,d,y;

   strcpy(s,gtk_entry_get_text(GTK_ENTRY(compose->entries[1])));
   if (!*s || 0==strcmp(s,"empty"))
   {
      Gtk_entry_set_text(GTK_ENTRY(compose->entries[1]),"empty");
      printf("recepient missing\n");
      gtk_entry_select_region(GTK_ENTRY(compose->entries[1]),0,-1);
      return;
   }
   strcpy(s,gtk_entry_get_text(GTK_ENTRY(compose->entries[0])));
   if (0==strcmp(s,"root@localhost"))
   {
      printf("illeagle sender adress\n");
      gtk_entry_select_region(GTK_ENTRY(compose->entries[0]),0,-1);
      return;
   }
   sprintf(s,"From: %s\nTo: %s\nCc: %s\nBcc: %s\nDate: ",gtk_entry_get_text(GTK_ENTRY(compose->entries[0])),gtk_entry_get_text(GTK_ENTRY(compose->entries[1])),gtk_entry_get_text(GTK_ENTRY(compose->entries[2])),gtk_entry_get_text(GTK_ENTRY(compose->entries[3])));
   // convert 'Sun Aug 4 18:17:03 2002' -> 'Sun, 04 Aug 2002 18:17:03 '
   time(&t);
   sscanf(ctime(&t),"%s %s %d %d:%d:%d %d",s2,s3,&d,&h,&m,&sec,&y);
   sprintf(s+strlen(s),"%s, %02d %s %04d %02d:%02d:%02d ",s2,d,s3,y,h,m,sec);
   i=get_timezone();
   if (i<0) strcat(s,"-"); else strcat(s,"+");
   sprintf(s+strlen(s),"%02d%02d\nSubject: %s\nX-Mailer-Frontend: gpe-mail (%s)\n",i/3600,(i/60)%60,gtk_entry_get_text(GTK_ENTRY(compose->entries[4])),VERSION);
   sprintf(s2,"%s/OUTBOX/",config.expanded_path);
   if (!file_exists(s2))
   {
      sprintf(s2,"mkdir -p %s/OUTBOX",config.expanded_path);
      printf("%s\n",s2);
      system(s2);
      do_folders();
   }
   i=0;
   do
   {
      i++;
      sprintf(s2,"%s/OUTBOX/%d.head",config.expanded_path,i);
   } while (file_exists(s2));
   f=(FILE *)fopen(s2,"w");
   fwrite(s,strlen(s),1,f);
   fclose(f);
   strcpy(s2+strlen(s2)-4,"body");
   f=(FILE *)fopen(s2,"w");
   gtk_text_buffer_get_iter_at_offset (compose->text_buffer, &start_iter, 0);
   gtk_text_buffer_get_iter_at_offset (compose->text_buffer, &end_iter, -1);
   sp=gtk_text_buffer_get_text(compose->text_buffer,&start_iter,&end_iter,0);
   sp=g_locale_from_utf8(sp,-1,NULL,NULL,NULL);
   fwrite(sp,strlen(sp),1,f);
   fclose(f);
   if (0==strcmp(self.current_folder,"OUTBOX")) gtk_idle_add(open_mailfolder,NULL);
   compose_window_destroy(button, compose);


}

void do_compose(GtkButton *button, gpointer user_data)
{
   char *q[]={"From","To","Cc","Bcc","Subj",NULL};
   int a;
   GtkWidget *l;
   GtkWidget *but1,*but2;
   char s[100];
   char s2[100];
   char **sp;
   char *sp2,*sp3;
   struct compose_t *compose;
   struct msg_body_t *body;

   body=(struct msg_body_t *)user_data;
   if (body==NULL) body=&self.body;

   compose=(struct compose_t *)malloc(sizeof(struct compose_t));
   if (compose==NULL)
   {
	   perror("malloc");
	   return;
   }
   compose->win=NULL;
   if (button!=(GtkButton *)self.composeb && (0==strcmp(self.current_folder,"OUTBOX"))) return;
   if (button!=(GtkButton *)self.composeb && self.cl1_row==-1) return;
   if ((NULL==compose->win)||!GTK_IS_WIDGET(compose->win))
   {
      compose->win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
      compose->vb1=gtk_vbox_new(FALSE,0);
      gtk_container_add(GTK_CONTAINER(compose->win),compose->vb1);
      compose->t=gtk_table_new(4,100,FALSE);
      gtk_box_pack_start (GTK_BOX (compose->vb1), compose->t, FALSE, FALSE, 0);
      for(a=0;q[a]!=NULL;a++)
      {
         sprintf(s,"%s:",q[a]);
         l=gtk_label_new(s);
         gtk_table_attach_defaults(GTK_TABLE(compose->t),l,1,2,a+1,a+2);
	 if (a>0&&a<4)
	 {
		 compose->entries[a]=gtk_colombo_new();
		 gtk_colombo_set_func(GTK_COLOMBO(compose->entries[a]),search_email);
	 }
	 else compose->entries[a]=gtk_entry_new();
         gtk_table_attach_defaults(GTK_TABLE(compose->t),compose->entries[a],2,3,a+1,a+2);
         if (0==strcmp(q[a],"From"))
            Gtk_entry_set_text(GTK_ENTRY(compose->entries[a]),get_config("Outgoing","Email Adress"));

      }
      gtk_signal_connect (GTK_OBJECT(compose->entries[4]), "changed", 
         (GtkSignalFunc) compose_window_update_title, compose);
      compose->text=gtk_text_view_new();
      compose->text_buffer=gtk_text_view_get_buffer (GTK_TEXT_VIEW (compose->text));
      gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(compose->text),GTK_WRAP_WORD);
      compose->sw=gtk_scrolled_window_new(NULL,NULL); 
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(compose->sw),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
      gtk_box_pack_start (GTK_BOX (compose->vb1), compose->sw, TRUE, TRUE, 0);
      gtk_widget_show(compose->sw);
      gtk_text_view_set_editable(GTK_TEXT_VIEW(compose->text),1);
      gtk_container_add (GTK_CONTAINER (compose->sw), compose->text);
      compose->hb=gtk_hbox_new(FALSE,0);
      gtk_box_pack_start (GTK_BOX (compose->vb1), compose->hb, FALSE, FALSE, 0);
      but1=mkbutton("cancel","Cancel","");
      gtk_signal_connect (GTK_OBJECT(but1), "clicked", 
         (GtkSignalFunc) compose_window_destroy, compose);
      but2=mkbutton("send","Send","");
      gtk_signal_connect (GTK_OBJECT(but2), "clicked", 
         (GtkSignalFunc) compose_window_send, compose);
      gtk_box_pack_start (GTK_BOX (compose->hb), but1, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (compose->hb), but2, TRUE, TRUE, 0);
   }
      gtk_window_set_title(GTK_WINDOW(compose->win),"Compose:");
   s2[0]=0;
   for(a=1;a<5;a++)
      Gtk_entry_set_text(GTK_ENTRY(compose->entries[a]),s2);
   if (button==GTK_BUTTON(body->forwardb))
   {
      sp=(char **)&s;
      if (0==gtk_clist_get_text((GtkCList *)self.cl1,self.cl1_row,0,sp)) {printf("can not find file name in clist\n");return;}
      while (**sp==' ') (*sp)++;
      if (starts_with(*sp,"Fwd:")) *sp+=4;
      if (starts_with(*sp,"fwd:")) *sp+=4;
      while (**sp==' ') (*sp)++;
      sprintf(s2,"Compose: Fwd: %s",*sp);
      //gtk_window_set_title(GTK_WINDOW(compose->win),g_locale_from_utf8(s2,-1,NULL,NULL,NULL));
      gtk_window_set_title(GTK_WINDOW(compose->win),s2);
      sprintf(s2,"Fwd: %s",*sp);
      Gtk_entry_set_text(GTK_ENTRY(compose->entries[4]),s2);
      s2[0]=0;
      Gtk_entry_set_text(GTK_ENTRY(compose->entries[1]),s2);
   }
   if (button==GTK_BUTTON(body->replyb))
   {
      sp=(char **)&s;
      if (0==gtk_clist_get_text((GtkCList *)self.cl1,self.cl1_row,0,sp)) {printf("can not find file name in clist\n");return;}
      while (**sp==' ') (*sp)++;
      if (starts_with(*sp,"Re:")) *sp+=3;
      if (starts_with(*sp,"re:")) *sp+=3;
      while (**sp==' ') (*sp)++;
      sprintf(s2,"Compose: Re: %s",*sp);
      //gtk_window_set_title(GTK_WINDOW(compose->win),g_locale_from_utf8(s2,-1,NULL,NULL,NULL));
      gtk_window_set_title(GTK_WINDOW(compose->win),s2);
      sprintf(s2,"Re: %s",*sp);
      Gtk_entry_set_text(GTK_ENTRY(compose->entries[4]),s2);
      sp=(char **)&s;
      if (0==gtk_clist_get_text((GtkCList *)self.cl1,self.cl1_row,1,sp)) {printf("can not find file name in clist\n");return;}
      Gtk_entry_set_text(GTK_ENTRY(compose->entries[1]),*sp);
   }
   gtk_text_buffer_set_text (compose->text_buffer, "", -1);
   if (button==GTK_BUTTON(self.composeb))
   {
      s2[0]=0;
      Gtk_entry_set_text(GTK_ENTRY(compose->entries[1]),s2);
      Gtk_entry_set_text(GTK_ENTRY(compose->entries[4]),s2);
   } else {
      gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)compose->text_buffer,"\n\nOriginal Message:\n",-1);
      gtk_text_buffer_get_iter_at_offset (body->tb, &start_iter, 0);
      gtk_text_buffer_get_iter_at_offset (body->tb, &end_iter, -1);
      sp2=gtk_text_buffer_get_text(body->tb,&start_iter,&end_iter,0);
      while (sp2!=NULL)
      {
         sp3=strchr(sp2,'\n');
         if (sp3!=NULL)
         {
      gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)compose->text_buffer,"> ",-1);
      gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)compose->text_buffer,sp2,sp3-sp2+1);
            sp2=sp3+1;
         } else sp2=NULL;
      }

   }
   gtk_text_thaw((GtkText *)compose->text);
   gtk_widget_show_all(compose->win);
}

void on_col_click(GtkCList *clist, gint column, gpointer user_data)
{
   if (column==2) gtk_clist_set_sort_column(clist,4);
   else gtk_clist_set_sort_column(clist,column);
   if (column==self.sortby_last)
      gtk_clist_set_sort_type(clist,1-clist->sort_type);
   self.sortby_last=column;
   gtk_clist_sort(clist);
}

void on_subj_sel (GtkCList *clist, gint row, gint column,
      GdkEventButton *event, gpointer user_data)
{
   char s[100];
   char **sp;
   char s2[100000];
   char s3[500];
   char boundary[500];
   char *sp2,*sp3;
   char type;
   char alternative;
   char open_in_dillo;
   char is_html;
   int i;
   char double_click;
   struct msg_body_t *body;
   char body_encoding_name[30];
   
   
   if (event==NULL) double_click=0;
   else
   {
   	// event->type==GDK_BUTTON_RELEASE : one click
   	// event->type==GDK_2BUTTON_PRESS : double click
   	if ((event->type!=GDK_BUTTON_RELEASE)&&
			(event->type!=GDK_2BUTTON_PRESS)) return;
   	double_click=(event->type==GDK_2BUTTON_PRESS);
   }

   if (!double_click) 
   {
   body=&self.body;
   body->win=self.w;
   } else {
      gtk_window_set_title(GTK_WINDOW(self.w),"gpe-mail");
      body=(struct msg_body_t *)malloc(sizeof(struct msg_body_t));
      if (body==NULL)
         {
         perror("malloc");
         return;
      }
      body->win=gtk_window_new(GTK_WINDOW_TOPLEVEL);
      body->vb=gtk_vbox_new(FALSE,0);
      gtk_container_add(GTK_CONTAINER(body->win),body->vb);
      body->hb=gtk_hbox_new(FALSE,0);
      gtk_box_pack_start (GTK_BOX (body->vb), body->hb, FALSE, FALSE, 0);

      //body->deleteb=mkbutton("mini-cross","Delete","");
      body->deleteb=mkbutton("delete","Delete","");
      gtk_box_pack_start (GTK_BOX (body->hb), body->deleteb, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT(body->deleteb), "clicked", 
         (GtkSignalFunc) do_delete, body);
      
      body->forwardb=mkbutton("forward","Fwd","Forward");
      gtk_box_pack_start (GTK_BOX (body->hb), body->forwardb, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT(body->forwardb), "clicked", 
         (GtkSignalFunc) do_compose, body);

      body->replyb=mkbutton("reply","Reply","");
      gtk_box_pack_start (GTK_BOX (body->hb), body->replyb, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT(body->replyb), "clicked", 
         (GtkSignalFunc) do_compose, body);

      body->sw=gtk_scrolled_window_new(NULL,NULL); 
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(body->sw),GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
      gtk_paned_add2 ((GtkPaned *)self.b2, body->sw );
      gtk_widget_show(body->sw);
      gtk_box_pack_start (GTK_BOX (body->vb), body->sw, TRUE, TRUE, 0);
      // mst text
      body->t=gtk_text_view_new();
      body->tb=gtk_text_view_get_buffer (GTK_TEXT_VIEW (body->t));
      gtk_text_view_set_editable(GTK_TEXT_VIEW(body->t),FALSE);
      gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(body->t),GTK_WRAP_WORD);
      gtk_container_add (GTK_CONTAINER (body->sw), body->t);

      body->closeb=mkbutton("close","Close","");
      gtk_box_pack_start (GTK_BOX (body->vb), body->closeb, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT(body->closeb), "clicked", 
         (GtkSignalFunc) msg_body_destroy, body);

      gtk_widget_show_all(body->win);
   }


   type=0;
   open_in_dillo=get_config_boolean("Misc","Open Alternative text/html as html in dillo");
   is_html=0;
   sp=(char **)&s;
   if (0==gtk_clist_get_text((GtkCList *)self.cl1,row,3,sp)) {printf("can not find file name in clist\n");gtk_text_thaw((GtkText *)body->t);return;}
   //printf("s=%s\n",*sp);
   //sscanf(*sp,"%s.",&i);
   //sscanf(*sp,"%s",&s3);
   sscanf(*sp,"%s",s3);
   if (ends_with(s3,".head")) s3[strlen(s3)-5]=0;
   else printf("%s does not end with '.head'\n",s3);
   if (column==5) // seen
   {

      sprintf(s,"%s/%s/%s.flags",config.expanded_path,self.current_folder,s3);
      f=(FILE *)fopen(s,"r");
      if (f!=NULL) 
      {
         s2[0]=0;
         fgets(s2,sizeof(s2),f);
	 if ((sp2=strstr(s2,"\\Seen"))!=NULL)
		 strcpy(sp2,sp2+5);
	 else
	 {
		 if (strlen(s2))
			 if (s2[strlen(s2-1)]!=' ') strcat(s2," ");
		 strcat(s2,"\\Seen");
	 }
	 fclose(f);
         f=(FILE *)fopen(s,"w");
	 if (f==NULL) perror(s);
	 else
	 {
		 fprintf(f,"%s",s2);
		 //printf("writing '%s'\n",s2);
		 fclose(f);
	 }
	 gtk_clist_set_text((GtkCList *)self.cl1,row,5,NULL!=strstr(s2,"Seen") ? "Y":"N");
         sprintf(s2,"echo '%sseen %s \"%s\"' >> %s/.onsync",NULL!=strstr(s2,"Seen") ? "":"un",s3,self.current_folder,config.expanded_path);
	 //printf("system(%s)\n",s2);
	 system(s2);
      } //else printf("'%s' not found\n",s);
      gtk_clist_unselect_row((GtkCList *)self.cl1,row,1);
      return;
   }
   strcpy(self.cur_msg,s3);
   gtk_text_buffer_set_text (body->tb, "", -1);
   //gtk_text_set_point((GtkText *)body->t,0);
   //gtk_text_forward_delete((GtkText *)body->t,1e9);
   //printf("row=%d\n",row);
   //printf("num=%s\n",self.cur_msg);
   self.cl1_row=row;
   sprintf(s2,"%s/%s/%s.head",config.expanded_path,self.current_folder,self.cur_msg);
   //printf("openning: '%s'\n",s2);
   f=(FILE *)fopen(s2,"r");
   boundary[0]=0;
   //heb=0;
   alternative=0;
   body_encoding_name[0]=0;
   if (f)
   {
      do
      {
         s2[0]=0;
         fgets(s2,sizeof(s2),f);
         if (!s2[0]) break;
	 //printf("s2=%s\n",s2);
         convert(s2);
	 if (starts_with(s2,"Content-Type: multipart/alternative")) alternative=1;
         if (NULL!=(sp2=strstr(s2,"boundary=\"")))
         {
            sp2+=strlen("boundary=\"");
            if (NULL!=strchr(sp2,'"'))
            {
               *strchr(sp2,'"')=0;
               strcpy(boundary,sp2);
	       //printf("boundary=%s\n",boundary);
            }
         }
	 //if (NULL!=(sp2=strstr(s2,"charset=\"windows-1255\""))) heb=1;
	 //if (NULL!=(sp2=strstr(s2,"charset="))) printf("%s\n",sp2);
	 if (NULL!=(sp2=strstr(s2,"charset="))) 
	 {
	    sp2+=strlen("charset=");
	    if (*sp2=='"') sp2++;
	    sp3=sp2;
	    while (*sp2 && *sp2!='"' && *sp2!='\n' && *sp2!='\a' && *sp2!=';' && *sp2!=',' )sp2++;
	    strncpy(body_encoding_name,sp3,(sp2-sp3));
	    body_encoding_name[sp2-sp3]=0;
	    //printf("encoding(at header): '%s'\n",body_encoding_name);
	 }
      } while (s2[0]);
      fclose(f);
   }
   strncpy(encoding_name,body_encoding_name,sizeof(body_encoding_name));
   //printf("%s alternative \n",alternative ? "has": "hasn't");
   if (!open_in_dillo) alternative=0;


   sprintf(s2,"%s/%s/%s.flags",config.expanded_path,self.current_folder,self.cur_msg);
   f=(FILE *)fopen(s2,"r");
   if (f!=NULL)
   {
	   fgets(s2,sizeof(s2),f);
	   fclose(f);
	   if (NULL==strstr(s2,"Seen") && (get_config_boolean("Misc","Mark msgs as seen when viewed")))
	   {
		   strcat(s2," \\Seen");
		   sprintf(s2+strlen(s2)+1,"%s/%s/%s.flags",config.expanded_path,self.current_folder,self.cur_msg);
		   f=(FILE *)fopen(s2+strlen(s2)+1,"w");
		   if (f==NULL) perror(s2+strlen(s2)+1);
		   else
		   {
			   fprintf(f,"%s",s2);
			   fclose(f);
			   sprintf(s2,"echo 'seen %s \"%s\"' >> %s/.onsync",self.cur_msg,self.current_folder,config.expanded_path);
			   system(s2);
			   gtk_clist_set_text((GtkCList *)self.cl1,row,5,"Y");
		   }
	   }
   }

   sprintf(s2,"%s/%s/%s.body",config.expanded_path,self.current_folder,self.cur_msg);
   strcpy(s3,self.fromto);
   if (0==gtk_clist_get_text((GtkCList *)self.cl1,row,1,sp)) {printf("can not find file name in clist\n");gtk_text_thaw((GtkText *)body->t);return;}
   strcat(s3,*sp);
   strcat(s3,"\nSubject: ");
   if (0==gtk_clist_get_text((GtkCList *)self.cl1,row,0,sp)) {printf("can not find file name in clist\n");gtk_text_thaw((GtkText *)body->t);return;}
   strcat(s3,*sp);
   //gtk_window_set_title(GTK_WINDOW(body->win),g_locale_from_utf8(*sp,-1,NULL,NULL,NULL));
   gtk_window_set_title(GTK_WINDOW(body->win),*sp);
   strcat(s3,"\nDate: ");
   if (0==gtk_clist_get_text((GtkCList *)self.cl1,row,2,sp)) {printf("can not find file name in clist\n");gtk_text_thaw((GtkText *)body->t);return;}
   strcat(s3,*sp);
   strcat(s3,"\n\n");
   //printf("s3=%s\n",s3);
   gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)body->tb,s3,-1);

   //printf("opening: '%s'\n",s2);
   f=(FILE *)fopen(s2,"r");
   if (f) 
   {
      s2[0]=0;
      if (boundary[0])
      {
         for (i=0;i<alternative+1;i++) do
         {
            s2[0]=0;
	    fgets(s2,sizeof(s2),f);
	    //if (s2[0]) printf("removing '%s'\n",s2);
         } while (s2[0]&&!strstr(s2,boundary));
	 type=0;
	 while (s2[0]&&s2[0]!='\n')
	 {
            s2[0]=0;
	    fgets(s2,sizeof(s2),f);
	    if ((sp2=strstr(s2,"Content-Transfer-Encoding: base64"))!=NULL) type='B';
	    if ((sp2=strstr(s2,"Content-Transfer-Encoding: quoted-printable"))!=NULL) type='Q';
	    //if (NULL!=(sp2=strstr(s2,"charset=\"windows-1255\""))) heb=1;
	    if (NULL!=(sp2=strstr(s2,"charset=")))
	    {
	       sp2+=strlen("charset=");
	       if (*sp2=='"') sp2++;
	       sp3=sp2;
	       while (*sp2 && *sp2!='"' && *sp2!='\n' && *sp2!='\a' && *sp2!=';' && *sp2!=',' )sp2++;
	       strncpy(body_encoding_name,sp3,(sp2-sp3));
	       body_encoding_name[sp2-sp3]=0;
	       //printf("encoding(at body): '%s'\n",body_encoding_name);
	    }
	    if (starts_with(s2,"Content-Type: text/html")) is_html=1;
	    //if (NULL!=(sp2=strstr(s2,"charset="))) printf("%s\n",sp2);
	 }
         strncpy(encoding_name,body_encoding_name,sizeof(body_encoding_name));
      }
      s2[0]=0;
      do
      {
         sp2=s2+strlen(s2);
         sp2[0]=0;
	 fgets(sp2,sizeof(s2)-strlen(s2),f);
	 if (!*sp2) break;
	 //printf(":%s",sp2);
	 if (boundary[0])
	 {
	 	if (NULL!=(sp3=strstr(sp2,boundary))) {sp2=sp3;*sp2=0;}
	 }
      } while (*sp2);
      if (type=='B') a2b_base64(s2);
      else if (type=='Q') q2b(s2);
      fclose(f);
      if (!is_html) open_in_dillo=0;
      if (open_in_dillo )
      {
	 sprintf(s,"dillo /tmp/gpe-mail-%d.html",(int)geteuid());
         f=(FILE *)fopen(s+6,"w");
	 strcat(s," &");
	 if (f==NULL) open_in_dillo=0;
	 else
	 {
		 if (!fwrite(s3,strlen(s3),1,f)) open_in_dillo=0;
		 if (!fwrite(s2,strlen(s2),1,f)) open_in_dillo=0;
		 fclose(f);
		 if (open_in_dillo)
		 {
   			gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)body->tb,"\n\n----------------------------\n Html mail opened in dillo\n----------------------------\n",-1);
		  	system(s);
		 }
	 }
      }
      if (! open_in_dillo )
	//printf("s2=%s\n",s2);
	my_locale_to_utf8(s2);
   	gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)body->tb,s2,-1);
      //printf("body:\n%s\n",s2);
   } else
  	   gtk_text_buffer_insert_at_cursor((GtkTextBuffer *)body->tb,"-----------------------------\n     body has not been\n  downloaded with message\n-----------------------------\n",-1);
   gtk_text_thaw((GtkText *)body->t);
}

void on_mi_act (GtkMenuItem *menuitem, gpointer user_data)
{
   //printf("%s\n",(char *)user_data);
   strcpy(self.current_folder,(char *)user_data);
   open_mailfolder();
   gtk_text_buffer_set_text(self.body.tb, "", -1);
}


void add_mailfolder(char *name)
{
   GtkWidget *mi;
   mi=gtk_menu_item_new_with_label(name);
   gtk_widget_show(mi);
   gtk_menu_prepend(GTK_MENU(self.foldermenu),mi);
   gtk_signal_connect (GTK_OBJECT(mi), "activate", 
      (GtkSignalFunc) on_mi_act, name);
}

int open_mailfolder()
{
   char path[100];
   char s[100];
   char subj[100];
   char from[100];
   char date[100];
   char fname[100];
   char seen[2];
   int num_new;
   int cnt;
   char *a[7]={subj,from,date,fname,s,seen,NULL};

   int i;

   num_new=0;
   cnt=0;
   self.cur_msg[0]=0;
   self.cl1_row=-1;
   usleep(10000);
   gtk_clist_clear(GTK_CLIST(self.cl1));
   strcpy(path,config.expanded_path);
   strcat(path,"/");
   strcat(path,self.current_folder);
   if (strcmp(self.current_folder,"OUTBOX"))
      strcpy(self.fromto,"From:");
   else
      strcpy(self.fromto,"To:");
   gtk_clist_set_column_title(GTK_CLIST(self.cl1),1,self.fromto);
   if (!file_exists(path))
   {
	   sprintf(s,"/bin/mkdir -p %s",path);
	   system(s);
   }
   dir=opendir(path);
   if (!dir) {perror(path);exit(0);} 
   direntptr=(void *)1;
   do
   {
      direntptr=readdir(dir);
      if (direntptr)
      {
         if (ends_with(direntptr->d_name,".head"))
         {
            sscanf(direntptr->d_name,"%d.head",&i);
            strcpy(s,path);
            strcat(s,"/");
            strcat(s,direntptr->d_name);
            //printf("opening: %s \n",s);
            f=(FILE *)fopen(s,"r");
            subj[0]=0;
            from[0]=0;
            date[0]=0;
            do
            {
               s[0]=0;
               fgets(s,sizeof(s),f);
	       //printf("s(before)=%s\n",s);
               convert(s);
	       //printf("s(after)=%s\n",s);
	       my_locale_to_utf8(s);
               if (starts_with(s,"Subject:")) strcpy(subj,s+9);
               if (starts_with(s,self.fromto)) strcpy(from,s+strlen(self.fromto)+1);
               if (starts_with(s,"Date:")) strcpy(date,s+6);
            } while (s[0]);
            trunc_newlines(subj);
            trunc_newlines(from);
            trunc_newlines(date);
            fclose(f);
	    sprintf(s,"%s/%d.flags",path,i);
	    seen[0]='?';
	    seen[1]=0;
	    f=(FILE *)fopen(s,"r");
	    s[0]=0;
	    if (f!=NULL)
	    {
	    	fgets(s,sizeof(s),f);
	    	fclose(f);
		seen[0]='N';
		if (strstr(s,"Seen")!=NULL) seen[0]='Y';
		else num_new++;
	    }
            sprintf(s,"%08d",i);
            strcpy(fname,direntptr->d_name);
            gtk_clist_append(GTK_CLIST(self.cl1),a);
	    cnt++;
         }
      }
   } while (direntptr);

   self.sortby_last=2;
   gtk_clist_set_sort_column(GTK_CLIST(self.cl1),4);
   gtk_clist_sort(GTK_CLIST(self.cl1));
   gtk_clist_thaw(GTK_CLIST(self.cl1));
   printf("%d msgs (%d new)\n",cnt,num_new);
   return FALSE;
}

int do_folders()
{
   char s[200];
   char inbox_exists=0;

   usleep(10000);
   if (!file_exists(config.expanded_path))
   {
	   sprintf(s,"/bin/mkdir -p %s",config.expanded_path);
	   system(s);
   }
   dir=opendir(config.expanded_path);
   if (!dir) {perror(config.expanded_path);exit(0);} 
   direntptr=(void *)1;
   do
   {
      direntptr=readdir(dir);
      if (direntptr)
      {
         if ((strcmp(direntptr->d_name,"..")!=0) &&
             (strcmp(direntptr->d_name,"." )!=0))
            add_mailfolder(direntptr->d_name);
	 if (0==strcmp(direntptr->d_name,"inbox")) inbox_exists=1;
      }
   } while (direntptr);
   if (!inbox_exists) add_mailfolder("inbox");
   
   strcpy(self.current_folder,"inbox");
   gtk_idle_add(open_mailfolder,NULL);
   return FALSE;
}

int main(int argc, char **argv)
{

   gchar *titles[]={"Subject","From","Sent","filename","uid","seen"};
   GtkStyle *gtk_style;

   loadconfig();
   tilde(config.path,config.expanded_path);
   strcat(config.expanded_path,(1-((*get_config("Incoming","Use pop3 instead of imap"))-1)) ? "/pop":"/imap");

   // set locale
   gtk_set_locale();
   
   // init gtk
   gtk_init(&argc, &argv);

   // main window
   self.w=gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_signal_connect (GTK_OBJECT(self.w), "delete-event", 
      (GtkSignalFunc) gtk_main_quit, NULL);
      //(GtkSignalFunc) gtk_true, NULL);
   gtk_signal_connect (GTK_OBJECT(self.w), "destroy", 
      (GtkSignalFunc) gtk_main_quit, NULL);


   self.b0=gtk_vbox_new(FALSE,0);
   gtk_container_add (GTK_CONTAINER (self.w), self.b0);
   gtk_widget_show(self.b0);

   self.hb2=gtk_hbox_new(FALSE,0);
     gtk_box_pack_start (GTK_BOX (self.b0), self.hb2, FALSE, FALSE, 0);
   gtk_widget_show(self.hb2);

   // subject/msg pane
   self.b2=gtk_vpaned_new();
   gtk_widget_show(self.b2);

   //subject sw
   self.sw2=gtk_scrolled_window_new(NULL,NULL);
   gtk_widget_set_usize(self.sw2,-1,80);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self.sw2),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
   gtk_widget_show(self.sw2);

   //msg body
   self.body.sw=gtk_scrolled_window_new(NULL,NULL); 
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self.body.sw),GTK_POLICY_NEVER,GTK_POLICY_ALWAYS);
   gtk_paned_add2 ((GtkPaned *)self.b2, self.body.sw );
   gtk_widget_show(self.body.sw);
   self.vb3=gtk_vbox_new(FALSE,0);
   gtk_widget_show(self.vb3);

   self.folderoption=gtk_option_menu_new();
   gtk_widget_show(self.folderoption);
   self.foldermenu=gtk_menu_new();
   gtk_widget_show(self.foldermenu);
   gtk_option_menu_set_menu((GtkOptionMenu *)self.folderoption,self.foldermenu);
     gtk_box_pack_start (GTK_BOX (self.vb3), self.folderoption, FALSE, FALSE, 0);
     gtk_paned_add1 ((GtkPaned *)self.b2, self.vb3);
     gtk_box_pack_start (GTK_BOX (self.vb3), self.sw2, TRUE, TRUE, 0);


   // buttons
   self.tt=gtk_tooltips_new();
   self.configb=mkbutton("configure_16_mail","Config","Configure Setting");
   self.conf.win=NULL;

   self.syncb=mkbutton("send-receive16","Sync","Send-Receive");
   self.sync.win=NULL;

     gtk_box_pack_start (GTK_BOX (self.b0), self.b2, TRUE, TRUE, 0);
     gtk_box_pack_start (GTK_BOX (self.hb2), self.configb, TRUE, TRUE, 0);
     gtk_box_pack_start (GTK_BOX (self.hb2), self.syncb, TRUE, TRUE, 0);


   gtk_signal_connect (GTK_OBJECT(self.configb), "clicked", 
      (GtkSignalFunc) do_config, NULL);
   gtk_signal_connect (GTK_OBJECT(self.syncb), "clicked", 
      (GtkSignalFunc) do_sync, NULL);

   //self.body.deleteb=mkbutton("mini-cross","Delete","");
   self.body.deleteb=mkbutton("delete","Delete","");
     gtk_box_pack_start (GTK_BOX (self.hb2), self.body.deleteb, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT(self.body.deleteb), "clicked", 
      (GtkSignalFunc) do_delete, NULL);

   self.composeb=mkbutton("edit","Compose","");
   //self.composeb=mkbutton("compose","Compose","");
     gtk_box_pack_start (GTK_BOX (self.hb2), self.composeb, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT(self.composeb), "clicked", 
      (GtkSignalFunc) do_compose, NULL);

   self.body.forwardb=mkbutton("forward","Fwd","Forward");
     gtk_box_pack_start (GTK_BOX (self.hb2), self.body.forwardb, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT(self.body.forwardb), "clicked", 
      (GtkSignalFunc) do_compose, NULL);

   self.body.replyb=mkbutton("reply","Reply","");
     gtk_box_pack_start (GTK_BOX (self.hb2), self.body.replyb, TRUE, TRUE, 0);
   gtk_signal_connect (GTK_OBJECT(self.body.replyb), "clicked", 
      (GtkSignalFunc) do_compose, NULL);

   
   gtk_tooltips_enable(self.tt);
   // subject clist
   self.cl1=gtk_clist_new_with_titles(6,titles);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),0,100);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),1,50);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),2,30);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),3,0);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),4,0);
   gtk_clist_set_column_visibility(GTK_CLIST(self.cl1),3,0);
   gtk_clist_set_column_visibility(GTK_CLIST(self.cl1),4,0);
   gtk_clist_set_column_width(GTK_CLIST(self.cl1),5,10);
   gtk_container_add (GTK_CONTAINER (self.sw2), self.cl1);
   self.sortby_last=-1;
   gtk_signal_connect (GTK_OBJECT(self.cl1), "select_row", 
      (GtkSignalFunc) on_subj_sel, NULL);
   gtk_signal_connect (GTK_OBJECT(self.cl1), "click_column", 
      (GtkSignalFunc) on_col_click, NULL);

   // mst text
   self.body.t=gtk_text_view_new();
   self.body.tb=gtk_text_view_get_buffer (GTK_TEXT_VIEW (self.body.t));
   gtk_text_view_set_editable(GTK_TEXT_VIEW(self.body.t),FALSE);
   gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self.body.t),GTK_WRAP_WORD);
   gtk_container_add (GTK_CONTAINER (self.body.sw), self.body.t);
   gtk_style=gtk_widget_get_style(self.body.t);





   gtk_widget_show_all(self.w);
   gtk_idle_add(do_folders,NULL);

   gtk_main();
   gtk_exit(0);
   return 0;
}

