#ifndef _FEEDEDIT_H
#define _FEEDEDIT_H

GtkWidget *create_feed_edit(void);

typedef enum
{
	FEED_TYPE,
	FEED_NAME,
	FEED_URL,
	FEED_FIELDNUM
}t_feedfield;

typedef enum
{
	FT_PROTECTED,
	FT_COMPRESSED,
	FT_UNCOMPRESED
}t_feedtype;

#endif /* _FEEDEDIT_H */
