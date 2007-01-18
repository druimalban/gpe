#ifndef GPE_CONF_DEVICE_H
#define GPE_CONF_DEVICE_H

typedef enum
{
	DEV_UNKNOWN = 0,
	DEV_IPAQ_SA,
	DEV_IPAQ_PXA,
	DEV_SIMPAD,
	DEV_ZAURUS_COLLIE,
	DEV_ZAURUS_POODLE,
	DEV_ZAURUS_SHEPHERED,
	DEV_ZAURUS_HUSKY,
	DEV_ZAURUS_CORGI,
	DEV_ZAURUS_SPITZ,
	DEV_ZAURUS_AKITA,
	DEV_ZAURUS_BORZOI,
	DEV_NOKIA_770,
	DEV_RAMSES,
	DEV_HW_INTEGRAL,
	DEV_CELLON_8000,
	DEV_NETBOOK_PRO,
	DEV_JOURNADA,
	DEV_SGI_O2,
	DEV_SGI_INDY,
	DEV_SGI_INDIGO2,
	DEV_SGI_OCTANE,
	DEV_X86,
	DEV_POWERPC,
	DEV_SPARC,
	DEV_ALPHA,
	DEV_HTC_UNIVERSAL,
	DEV_ETEN_G500,
	DEV_HW_SKEYEPADXSL,
	DEV_MAX
} DeviceID_t;

typedef enum {
	DEVICE_CLASS_NONE        = 0x00,
	DEVICE_CLASS_PDA         = 0x01,
	DEVICE_CLASS_PC          = 0x02,
	DEVICE_CLASS_TABLET      = 0x04,
	DEVICE_CLASS_CELLPHONE   = 0x08,
	DEVICE_CLASS_MDE         = 0x10
} DeviceFeatureID_t;

DeviceID_t device_get_id (void);
DeviceFeatureID_t device_get_features (DeviceID_t device_id);
const gchar *device_get_name (void);
const gchar *device_get_type (void);
gchar *device_get_specific_file (const gchar *basename);


#endif
