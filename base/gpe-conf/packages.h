#ifndef GPE_PACKAGES_H
#define GPE_PACKAGES_H

void do_package_update();
int do_package_install(const char *package, int remove);
int do_package_check(const char *package);
gboolean poll_log_pipe_generic(void callback(char*));

GtkWidget *Packages_Build_Objects();
void Packages_Free_Objects();
void Packages_Save();
void Packages_Restore();

#endif
