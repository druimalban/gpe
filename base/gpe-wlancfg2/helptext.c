/*

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Library General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <gtk/gtk.h>

#include "support.h"
#include "helptext.h"

void init_help()
{
	scheme_help=g_strdup(
	_("Scheme\n\n \
	\
	Different WLAN cards may use different settings. Also different \
	situations may require different settings. Using schemes it is possible to \
	define several profiles for different situations and hardware combinations. \
	The setting string may contain the globbing characters '*' and '?' for \
	matching.\n\n \
	\
	Schemes are identified by\n\n \
	\
	Scheme name\n\n \
	A simple string identifying a situation like e.g.\n \
	\thome\n \
	\toffice\n \
	\t*\n \
	Mostly defaults to *\n\n \
	\
	Socket \n\n \
	The device socket number, usually 0 or 1. \n \
	Mostly defaults to *\n\n \
	\
	Instance\n\n \
	???\n \
	Mostly defaults to *\n\n \
	\
	HW-Address\n\n \
	The hardware address of the card. This makes it possible to \
	identify different hardware vendors and to set hardware \
	specific settings. Here the globbing characters	help to \
	match a whole vendor device address range like e.g.\n \
	\t00:60:1D:*|*,*,*,00:02:2D:*\n \
	which matches almost all original Lucent Wavelan cards."));
	
	
	general_help=g_strdup(
	_("General Settings\n\n \
	Info:\n\n \
	\
	Free form informational string, has no function.\n\n \
	\
	ESSID:\n\n \
	\
	Network name, sometimes also called Domain ID. Identifies your network, most \
	WLAN cards get this automatically from the access point in managed mode, other \
	cards require to specify a name. In Ad-Hoc mode the same name \
	should be specified on all participating nodes.\n\n \
	\
	NWID:\n\n \
	\
	Network ID, specifies nodes belonging to the same cell.\n\n \
	\
	Mode:\n\n\
	\
	Valid choices are:\n\n \
	\
	Ad-Hoc\n\n\
	\
	\tSometimes also called\n \
	\tDemo mode, nodes\n \
	\twithout access point\n\n \
	\
	Managed\n\n \
	\
	\tManaged network with\n \
	\taccess point\n\n \
	\
	Master\n\n \
	\tThe nodes acts as master or\n \
	\taccess point, not supported\n \
	\tby all cards.\n\n \
	\
	Repeater\n\n \
	\tforwarder, not supported\n \
	\tby all cards.\n\n \
	\
	Secondary\n\n \
	\tThe node acts as secondary\n \
	\tor backup master, not\n \
	\tsupported by all cards.\n\n \
	\
	Auto\n\n \
	\
	\tTry to automatically\n \
	\tdetermine operation mode\n \
	\tor use card's default mode."));
	
	rfparams_help=g_strdup(
	_("RF Parameters \n\n \
	\
	Channel specification\n\n \
	The WaveLAN frequency band is \
	from 2.4GHz up to 2.4836GHz \
	is devided into up to 14 \
	channels. Not all cards support \
	all channels. \n \
	For Ad-Hoc mode a fixed \
	common channel / freqency for \
	the participating nodes has to be \
	specified. For some access \
	point - card combinations it can \
	be necessary to specify the \
	access point's control channel \
	here.\n \
	The channel can be either \
	specified by it's frequency or \
	channel number.\n\n\
	\
	Rate\n\n \
	Specifies the transmission rate, \
	depending on the card \
	1MBit/sec, or 11MBit/sec are \
	supported. Auto selects the \
	card's default rate."));
	
	wep_help=g_strdup(
	_("WEP Encryption\n\n \
	\
	Encryption\n\n \
	\
	On - Enable link encryption\n \
	Off - Disable link encryption\n\n \
	\
	Mode\n\n \
	\
	Open\n \
	\tAlso accept not encrypted\n \
	\tdata packets\n \
	Restricted\n \
	\tOnly accept correctly\n \
	\tencrypted data packets."));
	
	enckey_help=g_strdup(
	_("Encryption Keys\n\n \
	\
	Encryption keys can be specified either as HEX encoded strings like \n\
	\t0123-4567-89\n \
	or\n \
	\t0a2d46ff3c\n \
	or as simple strings which will be converted to numerical keys automatically.\n\n \
	\
	Up to 4 different keys can be configured but only one of them \
	can be active at a time."));
	
	expert_help=g_strdup(
	_("Expert settings\n\n \
	\
	All WLAN settings are performed through two utility programs \
	called 'iwconfig' and 'iwpriv'. Those utilities provide even more \
	options than are usually necessary to get your WLAN up and running. \
	If one of those seldomly options has to be used it can be specified \
	here as it would be on the utilitie's commandline.\n\n \
	\
	For example some Orinico based need an extra iwpriv option to \
	properly enable Ad-Hoc or\n\n \
	Master mode:\n \
	Ad-Hoc:\n \
	\tset_port3 1\n \
	Master:\n \
	\tset_port3 0\n \
	\
	Additionally options for the iwspy utility can be specified. \n\n \
	\
	Be aware of the fact that wrong options can cause the settings \
	process to fail, not only the additional settings!\n\n \
	\
	For more information on the iwconfig, iwspy and iwpriv \
	utilities please consult the extra documentation."));
	
	general_help_simple=g_strdup(
	_("Basic Settings\n\n\
ESSID:\n\n\
\
Network name, sometimes also called Domain ID. Identifies your network, most \
WLAN cards get this automatically from the access point in managed mode, other \
cards require to specify a name. In Ad-Hoc mode the same name \
should be specified on all participating nodes.\n\n \
\
Mode:\n\n\
\
Valid choices are:\n\n\
\
\tAd-Hoc\n\n\
\
\tSometimes also called\n\
\tDemo mode, nodes\n\
\twithout access point\n\n\
\
\tManaged\n\n\
\
\tManaged network with\n\
\taccess point\n\n \
\
Channel:\n\n\
\
Usually the wireless LAN card detects the channel automatically in \
infrastructure mode. \
In Ad-Hoc you need to set it to the same setting like any other \
node in the desired net.\
"));
}
