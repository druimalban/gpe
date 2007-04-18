/*
 * gpe-mini-browser v0.20
 *
 * Basic web browser based on gtk-webcore 
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*****************************************************************
			GENERAL INCLUDES
*****************************************************************/

/* General Defines */
#define HOME_PAGE "file:///usr/share/doc/gpe/mini-browser-index.html"
#define HOMEPAGE_SIZE 60
#define COMPLETION "/.gpe/gpe-mini-browser/browser-completion"
#define DB_NAME "/.gpe/gpe-mini-browser/bookmarks"
#define CONF_NAME "/.gpe/gpe-mini-browser/gpe-mini-browser.conf"
#define GPE_MINI_BROWSER_UA " gpe-mini-browser 0.20"

/*internationalisation */
#include <libintl.h>
#define _(String) gettext(String)

/* global variables because I could not find a better solution */
GtkToolItem *stop_reload_button; /* points to the stop/reload button and is used to change dynamically the button */
GtkListStore *completion_store;  /* points to the GtkList that stores all entries for the entrycompletion */
int urlbar_hidden;		/* is used to check the status of the urlbar hiding when going fullscreen TODO remove this!*/

/* contains the necessary data for passing urls between the different functions 
 *html points to the html rendering engine that is being used
 *entry points to the GtEntry that is used to input the url
*/
struct url_data {
GtkWidget *html;
GtkWidget *entry;
};

/* contains all the necessary data to make the progressbar work 
 *main_window points to the window in which the progressbar has to be shown
 *statusbox is de pop-up window element in which the progressbar will be shown
 *pbar points to the actual progressbar
 exists is used to check for an existing progressbar to avoid duplicates
*/
struct status_data {
GtkWidget *main_window;
GtkWidget *statusbox;
GtkWidget *pbar;
gboolean exists;
};

/* contains all the necessary data for the zoom functions 
 *html points to the existing rendering engine/widget that will be zoomed in/out
 *contains a pointer to the settings of that rendering engine
*/
struct zoom_data {
Webi *html;
WebiSettings *settings;
};

/* data needed to hide the urlbar 
 *urlbox points to the widget containing the urlbar
 hidden is used to check the status of the urlbar (hidden or not)
 *points to the button on the toolbar so it can be changed depending if the urlbar is hidden or not
*/
struct urlbar_data {
GtkWidget *urlbox;
int hidden;
GtkToolItem *hiding_button;
};

/* data container for adding a bookmark 
 *html points to the current html rendering engine/widget
 *entry points to an GtkEntry in the bookmark adding window
 *treeview points to the GtkTree that contains the current bookmark list so that it is updated immediately
 bookmark_type marks if the bookmark is an entry or the current url (will be used for categories later)
*/
struct bookmark_add {
Webi *html;
GtkWidget *entry;
GtkWidget *treeview;
int bookmark_type;
};

/* data container to fill in the bookmark tree 
 *html points the current html rendering engine/widget
 *treeview points to the GtkTree that contains the current bookmark list so that it is updated immediately	
*/
struct tree_action
{
Webi *html;
GtkWidget *treeview;
};

/* fullscreen information 
 *app points to the current window
 *toolbar points to the toolbar
 *urlbox points to the urlbox if there is one
 (all are used to hide everything)
*/
struct fullscreen_info
{
GtkWidget *app;
GtkWidget *toolbar;
GtkWidget *urlbox;
#ifdef HILDON
GtkWidget *hsep;
#endif
};

/********************************************************
	      interface call primitives 
	      see: interface-calls.c
*********************************************************/

/* show pop_up window for url input 
 * @param *show is the button that got pressed to pop up the dialog
 * @param *html points to the html rendering engine that is going to show the loaded url
 */
void show_url_window (GtkWidget *show, GtkWidget *html);

/* destroy the pop_up window 
 * @param *button is the button which causes the window to be destroyed 
 * @param *window is a pointer to the window that you want to have destroyed
 */
void destroy_window (GtkWidget *button, gpointer *window);

/* pop up the status window with status bar at the bottom of the screen 
 * @param html current html rendering engine that has triggered the load_start signal
 * @param status_data a pointer to a status_data struct that has been filled in
 */
void create_status_window (Webi *html, gpointer *status_data);

/* destroy the status window, making more screen estate available for viewing 
 * @param html current html rendering engine which has triggered the load_stop signal
 * @param status_data a pointer to a filled in status_data struct
 */
void destroy_status_window (Webi *html, gpointer *status_data);

/* make progressbar actually show progress 
 * @param html rendering engine that has triggered the status signal
 * @param status status information generated by the rendering engine (automatically filled in by the callback)
 * @param status_data is a pointer to a correctly filled in status_data struct 
*/
void activate_statusbar (Webi *html, WebiLoadStatus *status, gpointer status_data);

/* set the webpage title in as the window title 
 * @param html is the current html rendering egine from which we will fetch the currently displayed page
 * @param app_window pointer to the window that contains the rendering engine for which we want to set the title*/
void set_title (Webi *html, GtkWidget *app_window);

/* show the current and correct url in the urlbox (big screen only) 
 * @param html rendering engine to get the current url from
 * @param entrybox GtkEntryBox used to inoput the url that needs to be updated
 */
void update_text_entry (Webi *html, GtkWidget *entrybox);

/* show urlbar and extra buttons for big screens 
 * @param html redering widget which will be addressed when the user types in a (new) url
 * @param toolbar a pointer to the toolbar of the interface to add in the buttons related to the bigscreen interface
 * @param set a pointer to the settings for the rendering engine so that we can get the zoom buttons to work
 */
GtkWidget * show_big_screen_interface ( Webi *html, GtkWidget *toolbar, WebiSettings *set);

/* add zoom buttons 
 * @param html pointer to the html engine that will be affected by the zoom 
 * @param toolbar pointer to the toolbar to which the zoom buttons should be added
 * @param set pointer to a WebiSettings struct to control the html rendering engines settings
 */
void add_zoom_buttons (Webi *html, GtkWidget *toolbar, WebiSettings * set);

/* show urlbar 
 * @param html rendering engine that will react to what is entered in the urlbar
 */
GtkWidget * create_url_bar (Webi *html);

/* hide urlbar 
 * @param button default callback value which contains a pointer to the pressed button
 * @param urlbar_data a pointer to a correctly filled in urlbar_data struct
 */
void hide_url_bar (GtkWidget *button, struct urlbar_data *url_bar);

/* show bookmark window 
 * @param button default callback value which contains a pointer to the pressed button
 * @param html engine in which the selected bookmark will eventually be rendered
 */
void show_bookmarks (GtkWidget *button, Webi *html);

/* bookmark add dialog 
 * @param button default callback value which contains a pointer to the pressed button
 * @param data pointer to a correctly filled in bookmark_add struct
 */
void show_add_dialog (GtkWidget *button, gpointer *data);

/* show history 
 * @param button default callback value which contains a pointer to the pressed button
 * @param html engine in which the eventually selected history item will be rendered
 */
void show_history(GtkWidget *button, Webi *html);

/********************************************************
	       url loading and handling
	       see: loading-calls.c 
*********************************************************/

/* fetch the parsed url and send it to the renderer */
void fetch_url (const gchar *url, GtkWidget *html);
/* parse the url. automatically add the http://, if it is a file check if it exists */
const gchar * parse_url (const gchar *url);
/* get the url from the entry box (url bar or pop-up window */
void load_text_entry (GtkWidget *Button, gpointer *text);
/* accept cookies further improvements depend on gtk-webcore cookie handling */
void handle_cookie (Webi *html, WebiCookie *cookie, gpointer *data);
/* go forward when forward button is pressed */
void forward_func (GtkWidget *forward, GtkWidget *html);
/* go backward if the back button is pressed */
void back_func (GtkWidget *back, GtkWidget *html);
/* go to the home page */
void home_func (GtkWidget *home, GtkWidget *html);
/* stop or reload the current page depending on status */
void stop_reload_func (GtkWidget *reload, GtkWidget *html);

/******************************************************
	            misc handling 
		    see: misc-calls.c
*******************************************************/

/* delete one or several bookmarks */
void delete_bookmarks (GtkWidget *button, gpointer *data);
/* open bookmark and close the bookmark window */
void open_bookmarks (GtkWidget *button, gpointer *data);
/* toggle bookmark type */
void toggle_type (GtkWidget *button, gpointer *data);
/* add bookmark to list */
void add_bookmark_func (GtkWidget *button, gpointer *data);
/* set entry completion on an entry */
int set_entry_completion(GtkWidget *entry);
/* create entry completion model */
GtkTreeModel * create_completion_model (void);
/* save first 100 entries of the completion list to a file */
void save_completion (GtkWidget *window);
/* find out where to save the data */
gchar * history_location (void);
/* clear history */
void clear_history(GtkWidget *button);
/* fullscreen mode */
void set_fullscreen (GtkWidget *button, gpointer *fullscreen_info);
/* set selected bookmark as homepage */
void set_as_homepage (GtkWidget *button, gpointer *data);


/******************************************************
	            database backend handling 
		    see: db-backend.c
*******************************************************/

/* initialize bookmark db */
int start_db (void);
/* stop bookmark db */
void stop_db (void);
/* insert a new bookmark in the db */
int insert_new_bookmark (char *bookmark);
/* remove a bookmark from the db */
int remove_bookmark (char *bookmark);
/* fill in tree */
int load_db_data (void *tree, int argc, char **argv, char **columnNames);
/* load in data from db */ 
int refresh_or_load_db (GtkWidget *tree);
/* retrieve the page that has been bookmarked as home */
int get_bookmark_home (char *home);
/* function that fills in the *home pointer (it recieves the data from the get_bookmark_home function)*/
int return_bookmark_home (void *home, int argc, char **argv, char **columnNames);
/* sets the selected bookmark as home */
int set_bookmark_home (char *selected);


/******************************************************
	            settings handling 
		    see: settings.c
*******************************************************/

/* gets the proxy from the environment and set it */
void set_proxy(Webi *html, WebiSettings *ks);
/* set the settings to some sane  defaults for small screens */
void set_default_settings(Webi *html, WebiSettings *ks);
/* load settings from file and apply 
 * @param html the current html widget
 * @param ks the settings struct for the current widget
 * @return 1 on failure, 0 on success
 */
int parse_settings_file(Webi *html, WebiSettings * ks);
/* zoom in (basically making text size bigger)*/
void zoom_in(GtkWidget * zoom_in, gpointer *data);
/* zoom out (basically decreasing text size)*/
void zoom_out(GtkWidget * zoom_out, gpointer *data);
/* write current settings to file to be able to restore them later */
int write_settings_to_file(WebiSettings *ks);
/* save settings on exit */
void save_settings(GtkWidget *app, WebiSettings *ks);
/* apply settings */
void apply_settings(GtkWidget * app, gpointer * data);
/* show configuration window */
void show_configuration_panel(GtkWidget *window, gpointer *data);
/* callbacks for the configuration window */
void toggle_javascript (GtkWidget * button, WebiSettings * ks);
void toggle_images (GtkWidget * button, WebiSettings * ks);
void set_font_size (GtkEntry * entry, WebiSettings * ks);
void set_proxy_config (GtkEntry * entry, WebiSettings * ks);

