#ifndef _PASSWD_H
#define _PASSWD_H

#ifdef __cplusplus
extern "C"
{
#endif

	extern int update_passwd(const struct passwd *pw, char *crypt_pw);


#ifdef __cplusplus
}
#endif

#endif /* _PASSWD_H */
