#ifndef GPE_VTYPE_PRIV
#define GPE_VTYPE_PRIV

#include <libintl.h>

#include "gpe/vcal.h"
#include "gpe/vevent.h"
#include "gpe/vtodo.h"
#include "gpe/vcard.h"

#define _(x) gettext (x)

#define ERROR_DOMAIN() g_quark_from_static_string ("gpevtype")

#define ERROR_PROPAGATE(error, e) \
  do \
    { \
      g_set_error (error, ERROR_DOMAIN (), 0, "%s: %s", __func__, \
                   e->message); \
      g_error_free (e); \
    } \
  while (0)

#endif
