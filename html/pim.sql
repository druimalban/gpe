-- GPE Pim Schema v0.1.0 
-- There are an infinite number of ways of storing PIM data
-- this is supposed to be one of them
--
-- composed from a number of discussions on the GPE list
-- by Owen Cliffe <occ@cs.bath.ac.uk> 25/3/2002
-- discussion should take place on the GPE list <gpe-list@linuxtogo.org>

-- NB this schema does not include elements required for 
-- Syncing, these will be added later
-- 

-- Base Pim Schema
-- ---------------

-- Semantics of groups are that unless an entity is explicitly placed
-- inside a group they belong to the default group (which is implied
-- by X.group_id==NULL or 0

create table gpe_groups(
       id    int NOT NULL,
       name  text UNIQUE,

       PRIMARY KEY(id)
);



-- Links from GPE PIM entities to each other, or to external entities
-- the source is a local GPE URN of the form: 
-- gpe://[host]/[type]/id
-- where  [host] is a FQDN for an external host or empty for a local entity 
-- [type] is one of either "contacts","events","groups" or ....
-- id is an integer primary key for one of those groups.
-- and dest is any available URN which can either be a net-URL

create table gpe_links(
       id		int NOT NULL,
       description	text,
       sourceURN	text NOT NULL,
       destURN		text NOT NULL
)

-- ------------------------------------------------------------

-- Contact Management Schema

-- each of the possible classes of contact/address is referenced here
-- This includes address types, and other forms of contact type
-- system-defined contact types are:
-- ?? 
-- and have id's 1-100
-- system-defined address types are 
-- ??
-- and have id's 1000-2000
-- user-defined contact types have id: 2000-3000
-- user-defined address types have id: 3000-4000

create table gpe_contact_types(
       id		int NOT NULL,
       type_name	text,
       PRIMARY KEY(id)
);

-- each 'contactable entity' has an entry here
create table gpe_person(
       id		int NOT NULL,
       prefix		text,
       suffix		text,
       given_name	text, --\   any combination of these 
       other_names	text, -- |  may be the display name 
       org		text, --/   depending on what is present
       
       PRIMARY KEY(id)
);

-- associations between people and groups
create table gpe_person_groups(
       person_id	int NOT NULL,
       group_id		int NOT NULL,
       PRIMARY KEY(person_id,group_id)
);

-- each general contact field of each contact is  contained here. 
create table gpe_contacts(
       id		int NOT NULL,
       person_id	int NOT NULL,
       type_id		int NOT NULL,
       value		text ,
       PRIMARY KEY(id)
);

-- an address contact (This is a special case of a contact)
create table gpe_address (
  id			int NOT NULL,
  address_type_id       int NOT NULL, -- the type from contact type
  person_id		int NOT NULL, -- a person

  address_line_1        text, 
  address_line_2        text,
  address_line_3        text, 
  address_line_4        text, 
  city                  text, 
  province              text, --or state
  pcode                 text, --postal or zip code
  country               text,
  
  PRIMARY KEY(id)
);			

-- ------------------------------------------------------------

-- gpe events and todos (TBD)
