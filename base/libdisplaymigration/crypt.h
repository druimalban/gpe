#ifndef LIBDM_CRYPT_H
#define LIBDM_CRYPT_H

extern void libdm_crypt_create_hash (char *display, char *challenge, size_t len, char *result);
extern gboolean libdm_crypt_sign_hash (struct rsa_key *k, char *hash, gchar **result);
extern gboolean libdm_crypt_check_signature (struct rsa_key *k, char *hash, char *sigbuf);

#endif
