#ifndef GPE_PACKAGES_H
#define GPE_PACKAGES_H

void do_package_update();
int do_package_install(char *package);
int do_package_check(char *package);

GtkWidget *Packages_Build_Objects();
void Packages_Free_Objects();
void Packages_Save();
void Packages_Restore();

#endif
