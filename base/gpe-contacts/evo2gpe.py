#!/usr/bin/env python
# evo2gpe.py
# Dump Evolution's contacts DB as a set of SQL statements for GPE-Contacts
# Robert Mibus <mibus@handhelds.org> and Damien Tanner <dctanner@xoasis.com>
#
# This file was derived from evo2story.py, part of Storm.
# (Storm is a mini Evolution clone by Damien Tanner (dctanner@xoasis.com)
# (Original copyright/licence preserved)
#    Storm is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    Foobar is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with Storm; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import re, sys, os, dbhash

#if len(sys.argv) < 2:
#	print "evo2storm\nUsage: python evo2storm.py [ipaqusername]@[ipaqip]"
#	sys.exit(1)

def open_vcard(vcard_source):
	first_level = re.compile('BEGIN:VCARD([0-9A-Za-z \t\n\r<>\(\)=\.@\-:;,/\\\-_]*?)END:VCARD')
	vcard_output = re.findall(first_level, vcard_source)
	if len(vcard_output) > 0:
		return vcard_output[0]
	else:
		return ''

def get_vcard_data(vcard_source, tag):
	second_level = re.compile(tag + '([0-9A-Za-z \t\n\r<>\(\)=\.@\-:;,/\\\-_]*?)\r\n')
	vcard_output = re.findall(second_level, vcard_source)
	if len(vcard_output) > 0:
		return vcard_output[0]
	else:
		return ''

def insert_value (id, key, val):
	return "INSERT INTO contacts VALUES (" + str(id) + ",'" + key +"',\"" + val + "\");\n"

file = open("/tmp/contacts.sql", "w")
file.write ("drop table contacts;\n")
file.write ("drop table contacts_urn;\n")
file.write ("create table contacts (urn INTEGER NOT NULL, tag TEXT NOT NULL, value TEXT NOT NULL);\n")
file.write ("create table contacts_urn (urn INTEGER PRIMARY KEY);\n")
db = dbhash.open(os.environ['HOME'] + "/evolution/local/Contacts/addressbook.db", "r")

x = 0
z = len(db.keys())
db.first()
while x < z - 1:
	vcard_key = db.next()
	vcard_data = open_vcard(vcard_key[1])
	if vcard_data != '':
		file.write("insert into contacts_urn values (" + str(x+1) + ");\n")
		file.write(insert_value (x+1, "NAME", get_vcard_data(vcard_data, "FN:")))
		file.write(insert_value (x+1, "HOME.EMAIL", get_vcard_data(vcard_data, "EMAIL;INTERNET:")))
		file.write(insert_value (x+1, "NICKNAME", get_vcard_data(vcard_data, "NICKNAME:")))
		file.write(insert_value (x+1, "WORK.ORGANIZATION", get_vcard_data(vcard_data, "ORG:")))
#		file.write("<birthday>" + get_vcard_data(vcard_data, "BDAY:") + "</birthday>\n")
		file.write(insert_value (x+1, "HOME.WEBPAGE", get_vcard_data(vcard_data, "URL:")))
		file.write(insert_value (x+1, "HOME.MOBILE", get_vcard_data(vcard_data, "TEL;CELL:")))
		file.write(insert_value (x+1, "HOME.PHONE", get_vcard_data(vcard_data, "TEL;HOME:")))
		file.write(insert_value (x+1, "WORK.PHONE", get_vcard_data(vcard_data, "TEL;WORK;VOICE:")))
		file.write(insert_value (x+1, "COMMENT", get_vcard_data(vcard_data, "NOTE:")))
	x = x + 1

#os.system("rsync -e ssh /tmp/contacts.xml " + sys.argv[1] + ":/usr/local/share/storm/data/contacts.xml")
#os.system("rm /tmp/contacts.xml")
print "Sync complete!"
