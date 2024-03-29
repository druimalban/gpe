#ifndef GPE_SHIELD_BACKEND_H
#define GPE_SHIELD_BACKEND_H

#define STATE_RELATED 		(1<<0)
#define STATE_ESTABLISHED 	(1<<1)
#define STATE_INVALID	 	(1<<2)
#define STATE_NEW		 	(1<<3)

typedef enum
{
	TARGET_ACCEPT,
	TARGET_REJECT,
	TARGET_DROP
} 
rule_target_t;

typedef enum
{
	CHAIN_INPUT,
	CHAIN_OUTPUT,
	CHAIN_FORWARD
} 
rule_chain_t;

typedef enum 
{
	PROT_TCP,
	PROT_UDP,
	PROT_ICMP,
	PROT_ALL
}
rule_protocol_t;

typedef struct
{
	char name[255];
	char oldname[255];
	int status;
	
	rule_target_t target;
	rule_protocol_t protocol;
	rule_chain_t chain;
	u_int d_port, s_port;
	int state;
	int is_policy;
	char interface[16];
}
rule_t;


typedef enum
{
	PK_FRONT = 0xA0,
	PK_BACK = 0x0B
} 
pkmsgtype_t;


typedef enum
{
	CMD_NONE,
	CMD_ADD,
	CMD_REMOVE,
	CMD_SHUTDOWN,
	CMD_SET,
	CMD_CLEAR,
	CMD_LOAD,
	CMD_SAVE,
	CMD_CHANGE,
	CMD_CFG_LOAD,
	CMD_CFG_DONTLOAD,
	CMD_NO_IPTABLES
}
pkcommand_t;

typedef enum
{
	PK_STATUS,
	PK_COMMAND,
	PK_RULE,
	PK_FINISHED
}
pkcontent_t;

typedef struct
{
	rule_t rule;
	pkcommand_t command;
}
msg2front_t;

typedef struct
{
	pkcommand_t command;
	rule_t rule;
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

#define PK_SOCKET "/tmp/.gpe_shield_icm"
#define LOADRULES_MARK "/etc/gpe/gpe-shield-load"
#define CONFIGFILE	"/etc/access.conf"

extern int suidloop (int sock);
extern void do_rules_apply();
extern int do_load_rules();
extern void do_clear();
extern gboolean find_iptables ();

#endif
