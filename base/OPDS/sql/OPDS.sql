-- OPDS Database Schema v0.2.0
-- This is a proposed schema for storing Personel Data
-- Much of the design is based on the GPE PIM Schema v0.1.0 
-- composed by Owen Cliffe <occ@cs.bath.ac.uk> 25/3/2002
-- as found on http://gpe.handhelds.org/pim.sql
-- discussion should take place on the gpe list <gpe@handhelds.org>
-- This schema includes structures for storing PIM Configuration and 
-- Sync Data to be shared by GPE Applications
-- Composed by Dave A Hall dave@AdelieSolutions.com


-- ***************************************************
-- *               Resource Tables                   *
-- * The following tables provide the ability to     *
-- * define groups OPDS Resources                    *
-- ***************************************************

-- ** OPDS_RESOURCE **
-- Maps a URN to a Standard Resource Tables 

CREATE TABLE OPDS_RESOURCE (
  urn    unsigned int NOT NULL UNIQUE, -- Unique Resource Number.
  name   text,                         -- Display name for resource.
  srt_id unsigned int,                 -- Standard Resource Table ID.
  last_modified date,                  -- The date and time of the last 
                                       -- modification.    
  PRIMARY KEY(srt_id)
);

-- ** OPDS_LINK ** 
-- Maps links between multiple URNs.

CREATE TABLE OPDS_LINK (
  source_urn       unsigned int NOT NULL, -- Resource Linking.
  destination_urn  unsigned int NOT NULL, -- Linked Resource.
  description      text,                  -- Description of link.
  PRIMARY KEY( source_urn, destination_urn )
);

-- ** OPDS_RESOURCE_TABLE **
-- This table is used to identify a list
-- Of available Standard Resource Tables
CREATE TABLE OPDS_RESOURCE_TABLE (
  srt_id        unsigned int NOT NULL UNIQUE, -- The Standard Resource Table ID
  table_name    text NOT NULL,      -- Table name must be unique within location
  type          text NOT NULL,      -- The type of this table IE
                                    -- ^ [Contacts|Events|Notes] 
  format_id     text NOT NULL,      -- Data Format used for this table.
  description   text                -- Brief description of purpose.
);


-- ** OPDS_FORMAT **
-- This table is used to identify a list
-- Of used data formats
CREATE TABLE OPDS_FORMAT (
  format_id     text NOT NULL UNIQUE, -- Upper case identity string
  format_name   text NOT NULL,        -- Display Name of format.
  description   text,                 -- Brief description of purpose.
  application   text,                 -- Default display application.

  PRIMARY KEY (format_id)
);

INSERT INTO OPDS_FORMAT(format_id, format_name, description)
                VALUES('OPDSC', 'Contacts', 'Open Personal Data Standard Contacts Format');
INSERT INTO OPDS_FORMAT(format_id, format_name, description)
                VALUES('OPDSE', 'Events', 'Open Personal Data Standard Events Format');
INSERT INTO OPDS_FORMAT(format_id, format_name, description)
                VALUES('OPDSB', 'Banking', 'Open Personal Data Standard Banking Format');
INSERT INTO OPDS_FORMAT(format_id, format_name, description)
                VALUES('VCARD', 'vCard', 'The vCard Electronic Business Card standard.');
INSERT INTO OPDS_FORMAT(format_id, format_name, description)
                VALUES('VCAL', 'vCalendar', 'The  vCalendar Electronic Business Card standard.');
INSERT INTO OPDS_FORMAT(format_id, format_name, description)
                VALUES('NVP', 'Name Value Pair', 'A simple Name=Value pair protocol.');
INSERT INTO OPDS_FORMAT(format_id, format_name, description)
                VALUES('TXT', 'Plain Text', 'Simple text string. UTF-8 ?');
INSERT INTO OPDS_FORMAT(format_id, format_name, description)
                VALUES('HTML', 'HTML', 'Subset of HTML to allow flexible formatting.');

-- ***************************************************
-- *               Group Tables                      *
-- * The following tables provide the ability to     *
-- * define groups to allocate OPDS Resources to them *
-- ***************************************************

-- ** OPDS_GROUP **
-- Provides a Name and description link 
-- for each group within the OPDS dataset

CREATE TABLE OPDS_GROUP (
  group_id     unsigned int NOT NULL UNIQUE,  -- Group Identifier
  parent_group unsigned int,                  -- parent group id.
  name         text NOT NULL UNIQUE,          -- Display name
  description  text,                          -- Brief Description
      
  PRIMARY KEY(group_id)
);
	
-- ** OPDS_GROUP_MEMBER **
-- Links a URN with a group_id.  
-- A single URN may be a member of multiple 
-- groups each membership should be on a
-- separate row 
 
CREATE TABLE  OPDS_GROUP_MEMBER (
  urn       unsigned int NOT NULL, -- The OPDS Unique Resource Number
  group_id  unsigned int NOT NULL  -- Group Identifier
);

-- *********************************************
-- *          Synchronization Tables           *
-- * The following tables are dedicated to the *
-- * Synchronization of OPDS Resources          *
-- *********************************************

-- ** OPDS_SYNC_HOST Table **
-- This table is used to store information 
-- required to connect to remote hosts
-- This tables definition is far from 
-- finished further data will be required 
-- for an effective Syncing Application

CREATE TABLE OPDS_SYNC_HOST (
  host_id         unsigned int NOT NULL, -- Unique identifier for host system.
  host_name       text NOT NULL,         -- Unique name for host system.
  host_auth       text,                  -- Data required for host authorization.
                                         -- ^ Encryption should be handled by 
                                         -- ^ the Sync Client
  sync_ids_table  text,          -- Table used to store URN->Host UID pairs
  last_anchor     text,          -- Last Sync Anchor used for sanity checking
  next_anchor     text,          -- Next Sync Anchor used for sanity checking
  conflict_method text,          -- Method used to resolve conflicts
  host_protocols  text,          -- Protocol list understood by host CSV
  host_types      text,          -- List of OPDS Types accepted by this host.
  last_sync       date,          -- The Time and date of the most recent sync.

  PRIMARY KEY(host_id, host_name)
);
	  
-- ** OPDS_SYNC_GROUP Table **
-- This table is used to store information linking groups 
-- of data resources with hosts to be synced. A Sync Host 
-- may have multiple groups and a group may be synced with 
-- multiple hosts. 
-- When the Sync Application is run on a specific host it 
-- is required to select all groups from OPDS_SYNC_GROUP 
-- that link to the specified host_id then use the 
-- OPDS_GROUP_MEMBER table to pass data for each matching 
-- element from that group.

CREATE TABLE OPDS_SYNC_GROUP (
  host_id    unsigned int NOT NULL, -- Remote host id from OPDS_SYNC_HOST
  group_id   unsigned int NOT NULL, -- group id from OPDS_GROUP_MEMBER

  PRIMARY KEY(host_id, group_id)
);


-- *********************************************
-- *        Standard Resource Tables           *
-- * The following tables are dedicated to the *
-- * Storage of OPDS Data elements              *
-- *********************************************

-- ** CONTACT **
CREATE TABLE CONTACT (
  urn           unsigened int NOT NULL,  -- The OPDS Unique Resource Number
  element_name  text NOT NULL,           -- Name used for identity and display
  element_value text NOT NULL,           -- formated element data.

  PRIMARY KEY(urn)
);

INSERT INTO OPDS_RESOURCE_TABLE(srt_id, table_name, type, format_id, description)
                        VALUES(1, 'CONTACT', 'Contacts', 'OPDSC', 
                               'Address Book entries');

-- ** EVENT **
CREATE TABLE EVENT (
  urn           unsigned int NOT NULL, -- The OPDS Unique Resource Number
  element_name  text NOT NULL,         -- Name used for identity and display
  element_value text NOT NULL,         -- formated element data.

  PRIMARY KEY(urn)
);

INSERT INTO OPDS_RESOURCE_TABLE(srt_id, table_name, type, format_id, description)
                        VALUES(2, 'EVENT', 'Events', 'OPDSE', 
                               'Calendar entries');

-- ** NOTE **
CREATE TABLE NOTE (
  urn           unsigned int NOT NULL,  -- The OPDS Unique Resource Number
  element_name  text NOT NULL,          -- Name used for identity and display
  element_value text NOT NULL,          -- formated element data.

  PRIMARY KEY(urn)
);

INSERT INTO OPDS_RESOURCE_TABLE(srt_id, table_name, type, format_id, description)
                        VALUES(3, 'NOTE', 'Notes', 'HTML', 
                               'Notes');

-- ** TODO **
CREATE TABLE TODO (
  urn           unsigned int NOT NULL,  -- The OPDS Unique Resource Number
  element_name  text NOT NULL,          -- Name used for identity and display
  element_value text NOT NULL,          -- formated element data.

  PRIMARY KEY(urn)
);

INSERT INTO OPD_RESOURCE_TABLE(srt_id, table_name, type, format_id, description)
                        VALUES(4, 'TODO', 'To-Dos', 'OPDSE', 
                               'To-Do list entries');