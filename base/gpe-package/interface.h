#ifndef GPE_PACKAGES_IF_H
#define GPE_PACKAGES_IF_H

int mainloop (int argc, char *argv[]);

enum
{
	COL_INSTALLED,
	COL_NAME,
	COL_FEED,
	COL_DESCRIPTION,
	COL_STATUS,
	COL_VERSION,
	COL_COLOR,
	N_COLUMNS
};

#endif
