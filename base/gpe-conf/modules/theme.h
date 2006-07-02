#ifndef _THEME_H
#define _THEME_H

GtkWidget *Theme_Build_Objects();
void Theme_Save();
void Theme_Restore();
void task_change_background_image(void);

/* definitions taken from mbdesktop.h */
enum {
  BG_SOLID = 1,
  BG_TILED_PXM,
  BG_STRETCHED_PXM,
  BG_GRADIENT_HORIZ,
  BG_GRADIENT_VERT,
  BG_CENTERED_PXM,
};

#endif
