int send_string(FILE *f, const char *s, int l);
int recv_string(FILE *f, char *buf, int buflen);
int dprintf(int lvl, const char *fmt, ...);
void saslerr(int why, const char *what);
void saslfail(int why, const char *what);
