#ifndef DISPLAYMIGRATION_CRYPT_H
#define DISPLAYMIGRATION_CRYPT_H

struct rsa_key
{
  gcry_mpi_t n, e, d, p, q, u;
};

extern void displaymigration_crypt_create_hash (char *display, char *challenge, size_t len, char *result);
extern gboolean displaymigration_crypt_sign_hash (struct rsa_key *k, char *hash, gchar **result);
extern gboolean displaymigration_crypt_check_signature (struct rsa_key *k, char *hash, char *sigbuf);

#endif
