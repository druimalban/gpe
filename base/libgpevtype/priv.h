#ifndef GPE_VTYPE_PRIV
#define GPE_VTYPE_PRIV

#include <libintl.h>

#include "gpe/vcal.h"
#include "gpe/vevent.h"
#include "gpe/vtodo.h"
#include "gpe/vcard.h"

#define _(x) gettext (x)

#define ERROR_DOMAIN() g_quark_from_static_string ("gpevtype")

#endif
