/* Starts player */
PlayerT *madplay_start(char *filename);

/* Stop the player */
int madplay_stop(void);

/* pause player, True=pause, False=restart */
int madplay_pause(gboolean pause);

/* seek to a specific file position */
int madplay_seek(unsigned int seconds);
