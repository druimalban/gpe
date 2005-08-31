#ifndef UTILS_H
#define UTILS_H


osync_bool add_item (OSyncContext *ctx, const char *type, const char *data, gchar **error);

osync_bool modify_item (OSyncContext *ctx, const char *type, unsigned int uid, const char *data, gchar **error);

osync_bool delete_item (OSyncContext *ctx, const char *type, unsigned int uid, gchar **error);

osync_bool parse_value_modified (gchar *string, gchar **uid, gchar **modified);

osync_bool report_change (OSyncContext *ctx, gchar *type, gchar *uid, gchar *hash, gchar *data);


#endif /* UTILS_H */
