#ifndef GPE_PACKAGES_H
#define GPE_PACKAGES_H

#include <libipkg.h> /* types */

#define LEN_PARAMS 255
#define LEN_LIST  1024
#define LEN_STR1   255
#define LEN_STR2  1024
#define LEN_STR3   128

typedef enum
{
	PK_FRONT = 0xA0,
	PK_BACK = 0x0B
} 
pkmsgtype_t;


typedef enum
{
	CMD_NONE,
	CMD_INSTALL,
	CMD_REMOVE,
	CMD_PURGE,
	CMD_UPDATE,
	CMD_UPGRADE,
	CMD_LIST,
	CMD_FILES,
	CMD_SEARCH,
	CMD_STATUS,
	CMD_INFO
}
pkcommand_t;

typedef enum
{
	PK_STATUS,
	PK_QUESTION,
	PK_REPLY,
	PK_LIST,
	PK_ERROR,
	PK_INFO,
	PK_PKGINFO,
	PK_PACKAGESTATE,
	PK_COMMAND,
	PK_FINISHED	
}
pkcontent_t;

typedef struct
{
	int priority;
	char str1[LEN_STR1];
	char str2[LEN_STR2];
	char str3[LEN_STR3];
	pkg_state_status_t status;
}
msg2front_t;

typedef struct
{
	pkcommand_t command;
	char params[LEN_PARAMS];
	char list[LEN_LIST];
}
msg2back_t;

typedef struct
{
	pkmsgtype_t type;
	pkcontent_t ctype;
	union
	{
		msg2front_t tf;
		msg2back_t tb;
	}content;
}
pkmessage_t;

#define PK_SOCKET "/tmp/.gpe_packages_icm"

extern int suidloop (int sock);

#endif
