/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>
#include <linux/init.h>

/*
 * Constants.
 */

static char *snes232_name = "SNES Gamepad Interface";

/*
 * Per-Snes232 data.
 */

struct snes232 {
	struct input_dev dev[2];
};

/*
 * snes232_interrupt() is called by the low level driver when characters
 * are ready for us. We then buffer them for further processing, or call the
 * packet processing routine.
 */

static int button_map[16] =
{
	0,	/* 0 */
	0,	/* 1 */
	0,	/* 2 */
	0,	/* 3 */
	BTN_TR,	/* 4 */
	BTN_TL,	/* 5 */
	BTN_X,	/* 6 */
	BTN_A,	/* 7 */
	0,	/* 8 */   /* right */
	0,	/* 9 */   /* left */
	0,	/* 10 */  /* down */
	0,	/* 11 */  /* up */
	BTN_START,	/* 12 */
	BTN_SELECT,	/* 13 */
	BTN_Y,	/* 14 */
	BTN_B,	/* 15 */
};

static void snes232_interrupt(struct serio *serio, unsigned char data, unsigned int flags)
{
	struct snes232* snes232 = serio->private;
	struct input_dev *dev = (data & 0x40) ? &snes232->dev[1] : &snes232->dev[0];
	int btn = (data & 0xf);
	int down = (data & 0x80) ? 0 : 1;

	switch (btn) {
	case 8:
	case 9:
		/* synthesize X motion */
		input_report_abs(dev, ABS_X, down ? (btn == 8 ? 64 : -64) : 0);
		break;

	case 10:
	case 11:
		/* synthesize Y motion */
		input_report_abs(dev, ABS_Y, down ? (btn == 10 ? 64 : -64) : 0);
		break;

	default:
		/* just a button press */
		input_report_key(dev, button_map[btn], down);
		break;
	}
}

/*
 * snes232_disconnect() is the opposite of snes232_connect()
 */

static void snes232_disconnect(struct serio *serio)
{
	struct snes232* snes232 = serio->private;
	input_unregister_device(&snes232->dev[1]);
	input_unregister_device(&snes232->dev[0]);
	serio_close(serio);
	kfree(snes232);
}

/*
 * snes232_connect() is the routine that is called when someone adds a
 * new serio device. It looks for the Snes232, and if found, registers
 * it as an input device.
 */

static void snes232_connect(struct serio *serio, struct serio_dev *dev)
{
	struct snes232 *snes232;
	int i, d;

	if (serio->type != (SERIO_RS232 | SERIO_SNES232))
		return;

	if (!(snes232 = kmalloc(sizeof(struct snes232), GFP_KERNEL)))
		return;

	memset(snes232, 0, sizeof(struct snes232));

	serio->private = snes232;

	if (serio_open(serio, dev)) {
		kfree(snes232);
		return;
	}

	for (d = 0; d < 2; d++) {
		snes232->dev[d].evbit[0] = BIT(EV_KEY) | BIT(EV_ABS);	
		snes232->dev[d].keybit[LONG(BTN_A)] = BIT(BTN_A) | BIT(BTN_B) | BIT(BTN_X) | BIT(BTN_Y) | \
			BIT(BTN_TL) | BIT(BTN_TR) | \
			BIT(BTN_START) | BIT(BTN_SELECT);
		snes232->dev[d].absbit[0] = BIT(ABS_X) | BIT(ABS_Y);
		
		snes232->dev[d].name = snes232_name;
		snes232->dev[d].idbus = BUS_RS232;
		snes232->dev[d].idvendor = SERIO_SNES232;
		snes232->dev[d].idproduct = 0x0001;
		snes232->dev[d].idversion = 0x0100;
		
		for (i = 0; i < 2; i++) {
			snes232->dev[d].absmax[ABS_X+i] =  64;
			snes232->dev[d].absmin[ABS_X+i] = -64;
			snes232->dev[d].absflat[ABS_X+i] = 4;
		}
		
		snes232->dev[d].private = snes232;
		
		input_register_device(&snes232->dev[d]);

		printk(KERN_INFO "input%d: %s on serio%d\n", snes232->dev[d].number, snes232_name, serio->number);
	}
}

/*
 * The serio device structure.
 */

static struct serio_dev snes232_dev = {
	interrupt:	snes232_interrupt,
	connect:	snes232_connect,
	disconnect:	snes232_disconnect,
};

/*
 * The functions for inserting/removing us as a module.
 */

int __init snes232_init(void)
{
	serio_register_device(&snes232_dev);
	return 0;
}

void __exit snes232_exit(void)
{
	serio_unregister_device(&snes232_dev);
}

module_init(snes232_init);
module_exit(snes232_exit);

MODULE_LICENSE("GPL");
