void UpdateUI_cb(void);

typedef enum {PlayerStopped, PlayerPlaying, PlayerPaused} PlayerStateT;

typedef struct PlayerT {
	unsigned char PlayerState;
	int input_fd;		/* filedesc where input from player comes in	*/
	unsigned int PosSec;	/* current position in seconds			*/
	void *input_cb;		/* function called when input becomes available	*/
	gint input_cb_tag;	/* tag for gdk_input				*/
	pid_t player_pid;	/* the PID of the player process		*/
	struct PlayerT *(*player_start) (char *);
	PlayerStateT (*player_stop)(void);
	PlayerStateT (*player_pause)(gboolean);
	PlayerStateT (*player_seek)(unsigned int);
} PlayerT;
