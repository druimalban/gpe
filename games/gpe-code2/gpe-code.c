/*
 * GTK/MasterMind by Knut Arild Erstad, knute@ii.uib.no
 *
 * This code is public domain.  If you use part of or all of this code in
 * your own program, please credit me and/or send me an email.  You don't
 * _have_ to, but it would be nice.
 */

#include <stdlib.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <time.h>

#include <glib.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define _(x)  gettext(x)

static GtkWidget *align, *vb;

/*** Pixmaps ***/

GdkPixmap *bpix[10];  /* empty/colors1..8/hidden */
GdkPixmap *spix[3];   /* empty/black/white */
GdkPixmap *divpix[4]; /* lines, arrows ... */

/*** Field ***/

typedef struct
{
    int color;
    GtkWidget *widget;
} MmField;

void mmFieldSetColor( MmField *field, int color )
{
    assert( color>=0 && color<=8 );
    field->color = color;
    gtk_pixmap_set( GTK_PIXMAP(field->widget), bpix[color], NULL );
}

/*** SolutionSeq ***/

typedef struct
{
    MmField field[4];
    GtkWidget *box;
} MmSolutionSeq;

void mmSolutionSeqInit( MmSolutionSeq *sseq )
{
    int i;
    GtkWidget *pixmap;
    sseq->box = gtk_hbox_new( FALSE, 0 );
    for (i=0; i<4; i++) {
	/* random color */
	sseq->field[i].color = rand()%8 + 1;
	/* new pixmap with "hidden" image */
	sseq->field[i].widget = gtk_pixmap_new( bpix[9], NULL );
	gtk_box_pack_start( GTK_BOX(sseq->box), sseq->field[i].widget,
			    FALSE, FALSE, 0 );
	gtk_widget_show( sseq->field[i].widget );
    }
    pixmap = gtk_pixmap_new( divpix[3], NULL );
    gtk_box_pack_start( GTK_BOX(sseq->box), pixmap, FALSE, FALSE, 0 );
    gtk_widget_show( pixmap );
}

void mmSolutionSeqShow( MmSolutionSeq *sseq )
{
    int i;
    for (i=0; i<4; i++)
	gtk_pixmap_set( GTK_PIXMAP(sseq->field[i].widget),
			bpix[sseq->field[i].color], NULL );
}

void mmSolutionSeqNew( MmSolutionSeq *sseq )
{
    int i;
    for (i=0; i<4; i++) {
	/* random color */
	sseq->field[i].color = rand()%8 + 1;
	/* select pixmap with "hidden" image */
	gtk_pixmap_set( GTK_PIXMAP(sseq->field[i].widget), bpix[9], NULL );
    }
}

/*** BW ***/

typedef struct
{
    int color;
    GtkWidget *widget;
} MmBW;

void mmBWSetColor( MmBW *bw, int color )
{
    assert( color>=0 && color<=2 );
    bw->color = color;
    /* bw->widget = gtk_pixmap_new( spix[color], NULL ); */
    gtk_pixmap_set( GTK_PIXMAP(bw->widget), spix[color], NULL );
}

/*** Feedback ***/

typedef struct
{
    MmBW bw[4];
    GtkWidget *table;
} MmFeedback;

void mmFeedbackSetColors( MmFeedback *feedback, int blacks, int whites )
{
    int i=0;
    assert( blacks>=0 && whites>=0 && blacks+whites<=4 );
    /* set blacks */
    while (blacks>0) {
	mmBWSetColor( &feedback->bw[i++], 1 );
	blacks--;
    }
    /* set whites */
    while (whites>0) {
	mmBWSetColor( &feedback->bw[i++], 2 );
	whites--;
    }
    /* set the rest empty */
    while (i<4)
	mmBWSetColor( &feedback->bw[i++], 0 );
}

void mmFeedbackInit( MmFeedback *f )
{
    int i;
    f->table = gtk_table_new( 2, 2, FALSE );
    for (i=0; i<4; i++) {
	f->bw[i].color = 0;
	f->bw[i].widget = gtk_pixmap_new( spix[0], NULL );
	gtk_table_attach_defaults( GTK_TABLE(f->table), f->bw[i].widget,
				   i%2, i%2+1, i/2+1, i/2+2 );
	gtk_widget_show( f->bw[i].widget );
    }
}

void mmFeedbackNew( MmFeedback *f )
{
    int i;
    for (i=0; i<4; i++) {
	f->bw[i].color = 0;
	gtk_pixmap_set( GTK_PIXMAP(f->bw[i].widget), spix[0], NULL );
    }
}

/*** TrySeq ***/

typedef struct
{
    MmField field[4];
    MmFeedback feedback;
    GtkWidget *box;
    GtkWidget *eventbox[4];
    GtkWidget *arrowpix;
    gint handler[4];
} MmTrySeq;

void mmTrySeqInit( MmTrySeq *seq )
{
    int i;
    seq->box = gtk_hbox_new( FALSE, 0 );
    for (i=0; i<4; i++) {
	seq->field[i].color = 0;
	/*seq->field[i].widget = gtk_button_new_with_label( "0" );*/
	seq->eventbox[i] = gtk_event_box_new();
	gtk_box_pack_start( GTK_BOX(seq->box), seq->eventbox[i],
			    FALSE, FALSE, 0 );
	gtk_widget_show( seq->eventbox[i] );
	seq->field[i].widget = gtk_pixmap_new( bpix[0], NULL );
	gtk_container_add( GTK_CONTAINER(seq->eventbox[i]),
			   seq->field[i].widget );
	gtk_widget_show( seq->field[i].widget );
    }
    /* pixmap where arrow will be */
    seq->arrowpix = gtk_pixmap_new( divpix[1], NULL );
    gtk_box_pack_start( GTK_BOX(seq->box), seq->arrowpix, FALSE, FALSE, 0 );
    gtk_widget_show( seq->arrowpix );
    /* feedback */
    mmFeedbackInit( &seq->feedback );
    gtk_box_pack_start( GTK_BOX(seq->box), seq->feedback.table,
			FALSE, FALSE, 0 );
    gtk_widget_show( seq->feedback.table );
}

void mmTrySeqNew( MmTrySeq *seq )
{
    int i;
    for (i=0; i<4; i++) {
	seq->field[i].color = 0;
	gtk_pixmap_set( GTK_PIXMAP(seq->field[i].widget), bpix[0], NULL );
    }
    mmFeedbackNew( &seq->feedback );
}

/* a callback function for changing colors */

gint fieldChange( GtkWidget *widget, GdkEventButton *event, gpointer data )
{
    if (event->type == GDK_BUTTON_PRESS) {
	int i;
	MmField *f = data;
	i = (event->button == 2) ? 0 : (event->button == 1)
	    ? (f->color + 1) % 9 : (f->color + 8) % 9;
	mmFieldSetColor( f, i );
	return TRUE;
    }
    return FALSE;
}


void mmTrySeqSetActive( MmTrySeq *seq )
{
    int i;
    for (i=0; i<4; i++) {
	seq->handler[i] = gtk_signal_connect( GTK_OBJECT(seq->eventbox[i]),
					      "button_press_event",
					      GTK_SIGNAL_FUNC(fieldChange),
					      &seq->field[i] );
    }
    gtk_pixmap_set( GTK_PIXMAP(seq->arrowpix), divpix[2], NULL );
}

void mmTrySeqSetInactive( MmTrySeq *seq )
{
    int i;
    for (i=0; i<4; i++)
	gtk_signal_disconnect( GTK_OBJECT(seq->eventbox[i]), seq->handler[i] );
    gtk_pixmap_set( GTK_PIXMAP(seq->arrowpix), divpix[1], NULL );
}

int mmTrySeqEval( MmTrySeq *seq, MmSolutionSeq *solseq )
{
    int blacks, bws;
    int i;
    int tmp[4], stmp[4];

    /* copy seq's colors to tmp */
    for (i=0; i<4; i++) {
	tmp[i] = seq->field[i].color;
	stmp[i] = solseq->field[i].color;
    }
    /* evaluate blacks */
    blacks=0;
    for (i=0; i<4; i++)
	if (tmp[i] == stmp[i])
	    blacks++;
    /* evaluate blacks+whites */
    bws = 0;
    for (i=0; i<4; i++) {
	int j;
	for (j=0; j<4; j++)
	    if (tmp[i] == stmp[j]) {
		bws++;
		/* don't count this color anymore */
		tmp[i] = -1; stmp[j] = -2;
		break;
	    }
    }
    /* pass answers on to feedback */
    mmFeedbackSetColors( &seq->feedback, blacks, bws-blacks );
    /* return TRUE if correct */
    return (blacks==4) ? TRUE : FALSE;
}

/* Some global variables */
MmTrySeq seq[10];
MmSolutionSeq solseq;
int current_try = 0;

void newTry( void )
{
    if (current_try<10) {
	mmTrySeqSetInactive( &seq[current_try] );
	if (mmTrySeqEval( &seq[current_try], &solseq )) {
	    /* correct answer */
	    mmSolutionSeqShow( &solseq );
	    current_try = 10;
	    return;
	}
	current_try++;
	if (current_try<10) {
	    mmTrySeqSetActive( &seq[current_try] );
	} else {
	    /* 10 wrong answers -- failed */
	    mmSolutionSeqShow( &solseq );
	}
    }
}

void newGame( void )
{
    int i;
    /* Choose random colors for the solution seq. */
    mmSolutionSeqNew( &solseq );
    /* Clear all try seq's */
    for (i=0; i<10; i++)
	mmTrySeqNew( &seq[i] );
    /* current_try */
    if (current_try!=0) {
	if (current_try<10)
	    mmTrySeqSetInactive( &seq[current_try] );
	mmTrySeqSetActive( &seq[0] );
	current_try = 0;
    }
}

struct gpe_icon my_icons[] = {
  { "new", },
  { "icon", PREFIX "/share/pixmaps/gpe-code.png" },
  { "exit", },
  { "ok" },
  { "cancel" },
  /*{ "bempty", "code/bempty"},
  { "bred", "code/bred"},
  { "bgreen", "code/bgreen"},
  { "bblue", "code/bblue"},
  { "byellow", "code/byellow"},
  { "bhidden", "code/bhidden"},
  { "bcyan", "code/bcyan"},
  { "bbrown", "code/bbrown"},
  { "bwhite", "code/bwhite"},
  { "bblack", "code/bblack"},
  { "sempty", "code/sempty"},
  { "sblack", "code/sblack"},
  { "swhite", "code/swhite"},
  { "sline", "code/sline"},
  { "bline", "code/bline"},
  { "barrow", "code/barrow"},
  { "mm", "code/mm"},*/
  { NULL }
};

int main( int argc, char *argv[] )
{
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *buttonbox;
    GtkWidget *mainbox, *minorbox;
    GtkWidget *toolbar;
    GdkPixbuf *p;
    GtkWidget *pw;
    GtkWidget *scrolled_window;
    int i;

    gpe_application_init (&argc, &argv);

    bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
    textdomain (PACKAGE);

    srand (time (NULL));

    gpe_load_icons (my_icons);

    /* Make a window */
    window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    
    vb = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vb);
    
    gtk_window_set_title( GTK_WINDOW(window), "Mastermind" );
    gtk_signal_connect( GTK_OBJECT(window), "delete_event",
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL );
    
    toolbar = gtk_toolbar_new ();
    gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);  

    gtk_box_pack_start (GTK_BOX (vb), toolbar, TRUE, TRUE, 0);

    gtk_widget_realize (window);
    gpe_set_window_icon (window, "icon");
    gtk_window_set_title (GTK_WINDOW (window), _("CodeBreaker"));

    p = gpe_find_icon ("new");
    pw = gtk_image_new_from_pixbuf (p);
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
    			     _("New"), _("New game"), _("Tap here to start a new game."),
    			     pw, GTK_SIGNAL_FUNC (newGame), NULL);

    p = gpe_find_icon ("exit");
    pw = gtk_image_new_from_pixbuf (p);
    gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Exit"), 
    			     _("Exit"), _("Exit the program."), pw, 
    			     GTK_SIGNAL_FUNC (gtk_exit), NULL);

    align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
    gtk_box_pack_start (GTK_BOX (vb), align, TRUE, TRUE, 0);
  
    /*bpix[0] = gpe_render_icon (window->style, gpe_find_icon ("bempty"));
    bpix[1] = gpe_render_icon (window->style,gpe_find_icon ("bred"));
    bpix[2] = gpe_render_icon (window->style,gpe_find_icon ("bgreen"));
    bpix[2] = gpe_render_icon (window->style,gpe_find_icon ("bgreen" ));
    bpix[3] = gpe_render_icon (window->style,gpe_find_icon ("bblue" ));
    bpix[4] = gpe_render_icon (window->style,gpe_find_icon ("byellow" ));
    bpix[5] = gpe_render_icon (window->style,gpe_find_icon ("bcyan" ));
    bpix[6] = gpe_render_icon (window->style,gpe_find_icon ("bbrown" ));
    bpix[7] = gpe_render_icon (window->style,gpe_find_icon ("bwhite" ));
    bpix[8] = gpe_render_icon (window->style,gpe_find_icon ("bblack" ));
    bpix[9] = gpe_render_icon (window->style,gpe_find_icon ("bhidden" ));
    spix[0] = gpe_render_icon (window->style,gpe_find_icon ("sempty" ));
    spix[1] = gpe_render_icon (window->style,gpe_find_icon ("sblack" ));
    spix[2] = gpe_render_icon (window->style,gpe_find_icon ("swhite" ));
    divpix[0] = gpe_render_icon (window->style,gpe_find_icon ("sline" ));
    divpix[1] = gpe_render_icon (window->style,gpe_find_icon ("bline" ));
    divpix[2] = gpe_render_icon (window->style,gpe_find_icon ("barrow" ));
    divpix[3] = gpe_render_icon (window->style,gpe_find_icon ("mm" ));*/

    /* Load pixmaps */
#   define loadxpm(name) gdk_pixmap_create_from_xpm \
    ( window->window, NULL, NULL, PREFIX "/share/gpe/pixmaps/default/" PACKAGE "/" name )

    assert( window->window != NULL );
    
    bpix[0] = loadxpm( "bempty.xpm" );
    bpix[1] = loadxpm( "bred.xpm" );
    bpix[2] = loadxpm( "bgreen.xpm" );
    bpix[3] = loadxpm( "bblue.xpm" );
    bpix[4] = loadxpm( "byellow.xpm" );
    bpix[5] = loadxpm( "bcyan.xpm" );
    bpix[6] = loadxpm( "bbrown.xpm" );
    bpix[7] = loadxpm( "bwhite.xpm" );
    bpix[8] = loadxpm( "bblack.xpm" );
    bpix[9] = loadxpm( "bhidden.xpm" );
    spix[0] = loadxpm( "sempty.xpm" );
    spix[1] = loadxpm( "sblack.xpm" );
    spix[2] = loadxpm( "swhite.xpm" );
    divpix[0] = loadxpm( "sline.xpm" );
    divpix[1] = loadxpm( "bline.xpm" );
    divpix[2] = loadxpm( "barrow.xpm" );
    divpix[3] = loadxpm( "mm.xpm" );

    /* Make and fill in mainbox (vertical) */
    mainbox = gtk_vbox_new( FALSE, 0 );

    /* solution sequence */
    mmSolutionSeqInit( &solseq );
    gtk_box_pack_start( GTK_BOX(mainbox), solseq.box, TRUE, TRUE,  0 );
    gtk_widget_show( solseq.box );

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    /* Make and fill in minorbox (vertical) */
    minorbox = gtk_vbox_new( FALSE, 0 );
    
    /* try sequences */
    for (i=0; i<10; i++) {
	mmTrySeqInit( &seq[i] );
	gtk_box_pack_start( GTK_BOX(minorbox), seq[i].box, TRUE, TRUE, 0 );
        gtk_widget_show( seq[i].box );
    }
    
    gtk_widget_set_usize (scrolled_window, 190, 200);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW (scrolled_window), minorbox);
    gtk_box_pack_start( GTK_BOX(mainbox), scrolled_window, TRUE, TRUE, 0 );
	
    /* buttons */
    buttonbox = gtk_hbox_new( TRUE, 0 );
    /* try */
    button = gtk_button_new_with_label( "Try it" );
    gtk_signal_connect( GTK_OBJECT(button), "clicked",
			GTK_SIGNAL_FUNC(newTry), NULL );
    gtk_box_pack_start( GTK_BOX(buttonbox), button, TRUE, TRUE, 5 );
    gtk_widget_show( button );
    gtk_box_pack_start( GTK_BOX(mainbox), buttonbox, TRUE, TRUE,  5 );
    gtk_widget_show( buttonbox );

    /* finish off mainbox */
    gtk_widget_show( mainbox );
    gtk_container_add( GTK_CONTAINER(vb), mainbox );

    /* Show the window */
    gtk_widget_show_all( window );

    mmTrySeqSetActive( &seq[0] );

    /* Main loop */
    gtk_main();

    return 0;
}
