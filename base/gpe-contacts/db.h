#ifndef DB_H
#define DB_H

extern int db_open (void);
extern gboolean load_structure (void);

/* Well known tags */
#define TAG_PHONE_HOME		100
#define TAG_PHONE_WORK		101
#define TAG_PHONE_MOBILE	102
#define TAG_PHONE_FAX		103
#define TAG_PHONE_PAGER		104
#define TAG_PHONE_TELEX		105

#define TAG_ADDRESS_HOME	200
#define TAG_ADDRESS_WORK	201

#define TAG_INTERNET_EMAIL	300
#define TAG_INTERNET_WEB	301

#endif
