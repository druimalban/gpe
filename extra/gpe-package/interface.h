#ifndef GPE_PACKAGES_IF_H
#define GPE_PACKAGES_IF_H

int mainloop (int argc, char *argv[]);
void do_safe_exit();

/* Colors */

#define C_INSTALL 		"#88FF88"
#define C_INCOMPLETE 	"#FF4444"
#define C_REMOVE 		"#8888FF"

/* IDs for data storage fields */
enum
{
	COL_INSTALLED,
	COL_NAME,
	COL_FEED,
	COL_DESCRIPTION,
	COL_STATUS,
	COL_VERSION,
	COL_COLOR,
	COL_DESIREDSTATE,
	N_COLUMNS
};

#endif
