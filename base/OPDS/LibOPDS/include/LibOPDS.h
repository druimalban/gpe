/*******************************************************************************
 *   Copyright (C) 2002 David A Hall, Adelie Solutions All Rights Reserved     *
 *                                                                             *
 * This library is free software; you can redistribute it and/or modify it     *
 * under the terms of the GNU Lesser General Public License as published by    *
 * the Free Software Foundation; either version 2.1 of the License, or (at     *
 * your option) any later version.                                             *
 *                                                                             *
 * This library is distributed in the hope that it will be useful, but         *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY  *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public     *
 * License for more details. You should have received a copy of the GNU Lesser *
 * General Public License along with this library; if not, write to            *
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  *
 * 02111-1307  USA                                                             *
 *                                                                             *
 * Futher assistance and information concerning the LibOPDS API may be found   *
 * at http://www.AdelieSolutions.com/                                          *
 *                                                                             *
 * The author can be contacted via Email as Dave@AdelieSolutions.com           *
 * and at the time of writting at                                              *
 *                                                                             *
 * David A Hall                                                                *
 * Adelie Solutions                                                            *
 * 2712 Greystone Court,                                                       *
 *      AUSTELL,                                                               *
 *      GA 30106.                                                              *
 * Phone : 770 944 8971                                                        *
 *******************************************************************************/

/*******************************************************************************
 * This is a C version of the OPDS API. This program is designed to allow C    *
 * Programs to access the OPDS data schema residing in an SQLite database.     *
 *******************************************************************************/

#ifndef _OPDSAPI_C_
#define _OPDSAPI_C_

#include <glib.h>

/*******************
 * Group Functions *
 *******************/

/* opds_create_group */
/* Creates a resource grouping                                   */
/* returns a negative number is an error occured during creation */
/* returns the group_id of the new group if sucessful            */
guint 
opds_create_group( gchar *group_name,       /* Display Name of group */
		   gchar *db,               /* Master OPDS Database */
		   gchar *description=NULL, /* Group Description */
		   guint *parent_id=0       /* Parent Group Id */
		   );

/* opds_create_group */
/* Creates a resource grouping within a parent group             */
/* returns a negative number is an error occured during creation */
/* returns the group_id of the new group if sucessful            */
guint
opds_create_group( gchar *group_name, /* Display Name of group */
		   guint *parent_id,  /* Parent Group Id */
		   gchar *db          /* Master OPDS Database */
		   );

/* opds_update_group */
/* Updates a resource group name */
gboolean
opds_update_group( guint group_id,    /* Group Identifier */
		   gchar *group_name, /* Display Name of group */
		   gchar *db          /* Master OPDS Database */
		   );

/* opds_update_group */
/* Updates a resource group parent */
gboolean
opds_update_group( guint group_id,   /* Group Identifier */
		   guint *parent_id, /* Parent Group Id */
		   gchar *db         /* Master OPDS Database */
	   );

/* opds_get_group_id */
/* Returns the group id of a named group                         */
/* returns a negative number is an error occured during creation */
/* returns the group_id of the group if sucessful                */
guint
opds_get_group_id( gchar *group_name, /* Display Name of group */
		   gchar *db          /* Master OPDS Database */
		   );


/* opds_get_subgroups */
/* Returns an array of sub group group_ids for parent group id supplied */
/* Returns NULL if no sub groups are found                              */
/* If the parent_id 0 is supplied then all group ids will be returned.  */
GArray*
opds_get_subgroups( gchar *db,        /* Master OPDS Database */
		    guint parent_id=0 /* Parent Group Identifier */
		    );

/* opds_get_parent_group */
/* Returns an array of sub group group_ids for parent group id supplied */
/* Returns NULL if no sub groups are found                              */
/* If the parent_id 0 is supplied then all group ids will be returned.  */
glong
opds_get_parent_group( glong group_id, /* Group Identifier */
		       gchar *db       /* Master OPDS Database */
		       );

/* opds_group_resource */
/* place a Resource within the specified group. */
gboolean
opds_group_resource( guint urn,      /* Unique Resource Number(URN) */
		     guint group_id, /* Group Identifier            */
		     gchar *db       /* Master OPDS Database */
		     );

/* opds_ungroup_resource */
/* place a Resource within the specified group. */
gboolean
opds_group_resource( guint urn,      /* Unique Resource Number(URN) */
		     guint group_id, /* Group Identifier            */
		     gchar *db       /* Master OPDS Database */
		     );



/*******************
 * Table Functions *
 *******************/

/* opds_create_srt */
/* Creates a new Standard Resource Table                         */
/* returns a negative number is an error occured during creation */
/* returns the srt_id of the new table if sucessful              */
guint 
opds_create_srt( gchar *table,         /* Table Name */
		 gchar *type,          /* Table Type */
		 gchar *format_id,      /* Format Identifier */
		 gchar *db,            /* Master OPDS Database */
		 gchar *location=NULL  /* External OPDS datastore */
		                       /* Not yet implemented */
		 );

/********************
 * Format Functions *
 ********************/

/* opds_create_format */
/* Creates a resource format */
gboolean
opds_create_format( gchar *format_id,        /*Upper case identity string*/
		    gchar *format_name,      /* Short Name of format. */
		    gchar *db,               /* Master OPDS Database */
		    gchar *description=NULL, /* Brief description of purpose. */
		    gchar *application=NULL  /* Default display application. */
		    );

/* opds_create_format */
/* Creates a resource format                                     */
gboolean
opds_create_format( gchar *format_id,   /*Upper case identity string*/
		    gchar *format_name, /* Display Name of format. */
		    gchar *application, /* Default display application. */
		    gchar *db           /* Master OPDS Database */
		    );

/* opds_update_format */
/* Updates a resource format name */
gboolean 
opds_update_format( gchar *format_id,    /* Format Identifier */
		    gchar *format_name, /* Short Name of Format */
		    gchar *db           /* Master OPDS Database */
		    );

/* opds_update_format */
/* Updates a resource format application */
gboolean
opds_update_format( gchar *format_id,   /* Format Identifier */
		    gchar *application, /* Default display application. */
		    gchar *db           /* Master OPDS Database */
		    );

/* opds_delete_format */
/* Removes a resource format */
/* This function will automatically remove all URNs and TABLES */
/* Currently stored using the specified format. I assume you   */
/* know what your doing. If not maybe you should ignore this   */
/* function.                                                   */
gboolean
opds_delete_format( gchar *format_id,    /* Format Identifier */
		    gchar *db           /* Master OPDS Database */
		    );

/* opds_get_format_id */
/* Returns the format id of a named format.                      */
/* returns a negative number is an error occured during creation */
/* returns the format_id of the format if sucessful              */
gchar *
opds_get_format_id( gchar *format_name, /* Short Name of Format */
		    gchar *db           /* Master OPDS Database */



/******************
 * Link Functions *
 ******************/

/* opds_link_resource */
/* create a link between 2 Resources */
gboolean
opds_link_resources( guint source_urn,       /* URN of originating Resource */
		     guint dest_urn,         /* URN of linked Resource */
		     gchar *db,              /* Master OPDS Database */
		     gchar *description=NULL /* link description */
		     );

/* opds_unlink_resource */
/* removes a link between 2 Resources */
gboolean
opds_unlink_resources( guint source_urn,       /* URN of originating Resource */
		       guint dest_urn          /* URN of linked Resource */
		       gchar *db,              /* Master OPDS Database */
		       );



/**********************
 * Resource Functions *
 **********************/

/* opds_create_resource */
/* Creates a new Resource within the OPDS Master Database      */
/* returns a negative number is an error occured during insert */
/* returns the Unique Resource Number(URN) of the new resource */
/* if sucessful                                                */ 
glong opds_create_resource( gchar *name, /* Resource common name */ 
      		            gchar *table,/* Storage Table */
			    gchar *db    /* Master OPDS Database */
			    );

/* opds_insert_data */
/* Inserts a new data element into a specified resource.      */
/* If no table name is suppled then the OPDS_RESOURCE entery is   */
/* used to identify the required table. In this case the validity */
/* of the supplied URN is checked. If a table is supplied it is   */
/* the responsibility of the calling application to ensure valid  */
/* URNs are supplied. Supplying the table name is much faster.    */
/* Please see opds_find_table for futher info.                    */
gboolean 
opds_insert_resource( guint urn,             /* Unique Resource Number(URN) */
		      gchar *element_name,   /* Name of element to insert */
		      gchar *element_value,  /* Data Value */
		      gchar *db,             /* Master OPDS Database */
		      gchar *table=NULL      /* Table used to store element */
		      );

/* opds_update_resource */
/* Updates an existing data element within a specified resource.  */
/* If no table name is suppled then the OPDS_RESOURCE entery is   */
/* used to identify the required table.                           */
/* Supplying the table name is much faster.                       */
/* Please see opds_find_table for futher info.                    */
gboolean 
opds_update_resource( guint urn,            /* Unique Resource Number(URN) */
		      gchar *element_name,  /* Name of element to insert */
		      gchar *element_value, /* New Data Value */
   		      gchar *db,            /* Master OPDS Database */
		      gchar *table=NULL     /* Table used to store element */
		      );

/* opds_delete_resource */
/* Deletes an existing resource. from within the master db        */
/* this function removes all references to the urn including      */
/* those in OPDS_LINK, OPDS_GROUP_MEMBER and the table pointed    */
/* to within OPDS_RESOURCE                                        */
gboolean 
opds_delete_resource( guint urn, /* Unique Resource Number(URN) */
		      gchar *db  /* Master OPDS Database */
		      );

#endif
