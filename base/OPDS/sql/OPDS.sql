-- ODPS Database Schema v0.2.0
-- This is a proposed schema for storing ODPS Data
-- Much of the design is based on the ODPS PIM Schema v0.1.0 
-- composed by Owen Cliffe <occ@cs.bath.ac.uk> 25/3/2002
-- as found on http://ODPS.handhelds.org/pim.sql
-- discussion should take place on the ODPS list <ODPS@handhelds.org>
-- This schema includes structures for storing PIM Configuration and 
-- Sync Data to be shared by ODPS Applications
-- Composed by Dave A Hall dave@AdelieSolutions.com


-- ***************************************************
-- *               Resource Tables                   *
-- * The following tables provide the ability to     *
-- * define groups ODPS Resources                     *
-- ***************************************************

-- ** ODPS_RESOURCE **
-- Maps a URN to a Standard Resource Tables 

CREATE TABLE ODPS_RESOURCE (
  urn    int NOT NULL UNIQUE, -- Unique Resource Number.
  name   text,                -- Display name for resource.
  srt_id text,                -- Standard Resource Table ID.
  last_modified date,         -- The date and time of the last modification.    
      
  PRIMARY KEY(srt_id)
);

-- ** ODPS_LINK ** 
-- Maps links between multiple URNs.

CREATE TABLE ODPS_LINK (
  source_urn       text NOT NULL, -- Resource Linking.
  destination_urn  text NOT NULL, -- Linked Resource.
  description      text,          -- Description of link.
  PRIMARY KEY( source_urn, destination_urn )
);

-- ** ODPS_RESOURCE_TABLE **
-- This table is used to identify a list
-- Of available Standard Resource Tables
CREATE TABLE ODPS_RESOURCE_TABLE (
  srt_id        int NOT NULL UNIQUE, -- The Standard Resource Table ID
  table_name    text NOT NULL,       -- Table name must be unique within location
  type          text NOT NULL,       -- The type of this table IE
                                     -- ^ [Contacts|Events|Notes] 
  format_id     text NOT NULL,       -- Data Format used for this table.
  description   text                 -- Brief description of purpose.
);


-- ** ODPS_FORMAT **
-- This table is used to identify a list
-- Of used data formats
CREATE TABLE ODPS_FORMAT (
  format_id     text NOT NULL UNIQUE, -- Upper case identity string
  format_name   text NOT NULL,        -- Display Name of format.
  description   text,                -- Brief description of purpose.
  application   text,                -- Default display application.
                                     -- ^ a NULL entry indicate this db file.
  PRIMARY KEY (format_id)
);

INSERT INTO ODPS_FORMAT(format_id, format_name, description)
                VALUES('OPDSC', 'Contacts', 'Open Personal Data Standard Contacts Format');
INSERT INTO ODPS_FORMAT(format_id, format_name, description)
                VALUES('OPDSE', 'Events', 'Open Personal Data Standard Events Format');
INSERT INTO ODPS_FORMAT(format_id, format_name, description)
                VALUES('OPDSB', 'Banking', 'Open Personal Data Standard Banking Format');
INSERT INTO ODPS_FORMAT(format_id, format_name, description)
                VALUES('VCARD', 'vCard', 'The vCard Electronic Business Card standard.');
INSERT INTO ODPS_FORMAT(format_id, format_name, description)
                VALUES('VCAL', 'vCalendar', 'The  vCalendar Electronic Business Card standard.');
INSERT INTO ODPS_FORMAT(format_id, format_name, description)
                VALUES('NVP', 'Name Value Pair', 'A simple Name=Value pair protocol.');
INSERT INTO ODPS_FORMAT(format_id, format_name, description)
                VALUES('TXT', 'Plain Text', 'Simple text string. UTF-8 ?');
INSERT INTO ODPS_FORMAT(format_id, format_name, description)
                VALUES('HTML', 'HTML', 'Subset of HTML to allow flexible formatting.');

-- ***************************************************
-- *               Group Tables                      *
-- * The following tables provide the ability to     *
-- * define groups to allocate ODPS Resources to them *
-- ***************************************************

-- ** ODPS_GROUP **
-- Provides a Name and description link 
-- for each group within the ODPS dataset

CREATE TABLE ODPS_GROUP (
  group_id     int NOT NULL UNIQUE,  -- Group Identifier
  parent_group int,                  -- parent group id.
  name         text NOT NULL UNIQUE, -- Display name
  description  text,                 -- Brief Description
      
  PRIMARY KEY(group_id)
);
	
-- ** ODPS_GROUP_MEMBER **
-- Links a URN with a group_id.  
-- A single URN may be a member of multiple 
-- groups each membership should be on a
-- separate row 
 
CREATE TABLE  ODPS_GROUP_MEMBER (
  urn       int NOT NULL, -- The ODPS Unique Resource Number
  group_id  int NOT NULL  -- Group Identifier
);

-- *********************************************
-- *          Synchronization Tables           *
-- * The following tables are dedicated to the *
-- * Synchronization of ODPS Resources          *
-- *********************************************

	-- ** ODPS_SYNC_HOST Table **
	-- This table is used to store information 
	-- required to connect to remote hosts
	-- This tables definition is far from 
	-- finished further data will be required 
	-- for an effective Syncing Application

    	CREATE TABLE ODPS_SYNC_HOST (
          host_id         int NOT NULL, -- Unique identifier for host system.
          host_name       text NOT NULL,-- Unique name for host system.
          host_auth       text,         -- Data required for host authorization.
                                     -- ^ Encryption should be handled by the Sync
                                     -- ^ Client
	  last_anchor     text,          -- Last Sync Anchor used for sanity checking
	  next_anchor     text,          -- Next Sync Anchor used for sanity checking
	  conflict_method text,          -- Method used to resolve conflicts
	  host_protocols  text,          -- Protocol list understood by host CSV
	  host_types      text,          -- List of ODPS Types accepted by this host.
	  last_sync       date,          -- The Time and date of the most recent sync.

	  PRIMARY KEY(host_id, host_name)
	);
	  
	-- ** ODPS_SYNC_GROUP Table **
	-- This table is used to store information linking groups 
	-- of data resources with hosts to be synced. A Sync Host 
	-- may have multiple groups and a group may be synced with 
	-- multiple hosts. 
	-- When the Sync Application is run on a specific host it 
	-- is required to select all groups from ODPS_SYNC_GROUP 
	-- that link to the specified host_id then use the 
	-- ODPS_GROUP_MEMBER table to pass data for each matching 
	-- element from that group.

    	CREATE TABLE ODPS_SYNC_GROUP (
          host_id    int NOT NULL, -- Remote host id from ODPS_SYNC_HOST
          group_id   int NOT NULL, -- group id from ODPS_GROUP_MEMBER

	  PRIMARY KEY(host_id, group_id)
	);


-- *********************************************
-- *        Standard Resource Tables           *
-- * The following tables are dedicated to the *
-- * Storage of ODPS Data elements              *
-- *********************************************

-- ** CONTACT **
CREATE TABLE CONTACT (
  urn           int NOT NULL,  -- The ODPS Unique Resource Number
  element_name  text NOT NULL, -- Name used for identity and display
  element_value text NOT NULL, -- formated element data.

  PRIMARY KEY(urn)
);

INSERT INTO ODPS_RESOURCE_TABLE(srt_id, table_name, type, format_id, description)
                        VALUES(1, 'CONTACT', 'Contacts', 'OPDSC', 
                               'Address Book entries');

-- ** EVENT **
CREATE TABLE EVENT (
  urn           int NOT NULL,  -- The ODPS Unique Resource Number
  element_name  text NOT NULL, -- Name used for identity and display
  element_value text NOT NULL, -- formated element data.

  PRIMARY KEY(urn)
);

INSERT INTO ODPS_RESOURCE_TABLE(srt_id, table_name, type, format_id, description)
                        VALUES(2, 'EVENT', 'Events', 'OPDSE', 
                               'Calendar entries');

-- ** NOTE **
CREATE TABLE NOTE (
  urn           int NOT NULL,  -- The ODPS Unique Resource Number
  element_name  text NOT NULL, -- Name used for identity and display
  element_value text NOT NULL, -- formated element data.

  PRIMARY KEY(urn)
);

INSERT INTO ODPS_RESOURCE_TABLE(srt_id, table_name, type, format_id, description)
                        VALUES(3, 'NOTE', 'Notes', 'HTML', 
                               'Notes');

-- ** TODO **
CREATE TABLE TODO (
  urn           int NOT NULL,  -- The ODPS Unique Resource Number
  element_name  text NOT NULL, -- Name used for identity and display
  element_value text NOT NULL, -- formated element data.

  PRIMARY KEY(urn)
);

INSERT INTO ODPS_RESOURCE_TABLE(srt_id, table_name, type, format_id, description)
                        VALUES(4, 'TODO', 'To-Dos', 'OPDSE', 
                               'To-Do list entries');