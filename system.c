/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 1995, 1996  Robert Gentleman and Ross Ihaka
 *  Copyright (C) 1997--2004  Robert Gentleman, Ross Ihaka
 *                            and the R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

         /* See ../unix/system.txt for a description of functions */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gnome.h>
#include <glade/glade.h>
#include <libgnome/libgnome.h>

#if defined(HAVE_LOCALE_H)
#include <locale.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#include <Rversion.h>

#define R_INTERFACE_PTRS 1
#include <Rinterface.h>
#include <Rdevices.h>    /* for KillAllDevices */
#include <R_ext/Print.h> /* for Rprintf */

#include "gtkconsole.h"
#include "terminal.h"
#include "terminal-prefs.h" /* declares R_set_SaveAction */

#include "system.h" /* routines in other files to assign to interface ptrs */

	/*--- Initialization Code ---*/

extern SA_TYPE SaveAction;
extern SA_TYPE RestoreAction;

static void Rgnome_CleanUp(SA_TYPE saveact, int status, int runLast);

/*
 *  1) FATAL MESSAGES AT STARTUP
 */

static void Rgnome_Suicide(char *s)
{
    GtkWidget *dialog;
    gchar *message;

    /* Create the error message */
    message = g_strdup_printf("R: Fatal error\n\n%s", s);

    dialog = gnome_message_box_new(message,
				   GNOME_MESSAGE_BOX_ERROR,
				   GNOME_STOCK_BUTTON_CLOSE,
				   NULL);

    if(R_gtk_main_window != NULL)
	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(R_gtk_main_window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);

    gnome_dialog_run_and_close(GNOME_DIALOG(dialog));

    Rgnome_CleanUp(SA_SUICIDE, 2, 0);
}



/*
 *  3) ACTIONS DURING (LONG) COMPUTATIONS
 */

static void Rgnome_Busy(int which)
{
    if(which == 1) {
	gnome_appbar_set_default(GNOME_APPBAR(GNOME_APP(R_gtk_main_window)->statusbar), "Working...");
	while(gtk_events_pending()) gtk_main_iteration();
    } else {
	gnome_appbar_set_default(GNOME_APPBAR(GNOME_APP(R_gtk_main_window)->statusbar), "");
    }    
}

/*
 *  4) INITIALIZATION AND TERMINATION ACTIONS
 */

/*
   R_CleanUp is invoked at the end of the session to give the user the
   option of saving their data.
   If ask == SA_SAVEASK the user should be asked if possible (and this
   option should not occur in non-interactive use).
   If ask = SA_SAVE or SA_NOSAVE the decision is known.
   If ask = SA_DEFAULT use the SaveAction set at startup.
   In all these cases run .Last() unless quitting is cancelled.
   If ask = SA_SUICIDE, no save, no .Last, possibly other things.
 */

static void Rgnome_CleanUp(SA_TYPE saveact, int status, int runLast)
{
    GtkWidget *dialog;
    gint which; /* yes = 0, no = 1, cancel = 2 || -1 */
    char *tmpdir, buf[1024];

/*
    GList *curfile = R_gtk_editfiles;
    R_gtk_edititem *edititem;
*/
    if(saveact == SA_DEFAULT) /* The normal case apart from R_Suicide */
	saveact = SaveAction;

    if(saveact == SA_SAVEASK) {
	if(R_Interactive) { /* This is always interactive ... */
	    R_ClearerrConsole();
	    R_FlushConsole();
	    dialog = gnome_message_box_new(
		"Do you want to save your workspace image?\n\n"

		"Choose Yes to save an image and exit, choose\n"
		"No to exit without saving, or choose Cancel to\n"
		"return to R.",
		GNOME_MESSAGE_BOX_QUESTION,
		GNOME_STOCK_BUTTON_YES,
		GNOME_STOCK_BUTTON_NO,
		GNOME_STOCK_BUTTON_CANCEL,
		NULL);
	    
	    gnome_dialog_set_parent(GNOME_DIALOG(dialog), 
				    GTK_WINDOW(R_gtk_main_window));
	    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	    gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);
	    
	    which = gnome_dialog_run_and_close(GNOME_DIALOG(dialog));
	    switch(which) {
	    case 0:
		saveact = SA_SAVE;
		break;
	    case 1:
		saveact = SA_NOSAVE;
		break;
	    default:
		jump_to_toplevel();
		break;
	    }
	}
	else saveact = SaveAction;
    }

    switch (saveact) {
    case SA_SAVE:
	if(runLast) R_dot_Last();
	if(R_DirtyImage) R_SaveGlobalEnv();
	if(R_Interactive)
	    gtk_console_save_history(GTK_CONSOLE(R_gtk_terminal_text), 
				     R_HistoryFile, R_HistorySize, NULL);
	break;
    case SA_NOSAVE:
	if(runLast) R_dot_Last();
	break;
    case SA_SUICIDE:
    default:
	break;
    }

    R_RunExitFinalizers();

    /* save GUI preferences */
    R_gnome_prefs_save();

/* unlink all the files we opened for editing 
    while(curfile != NULL) {
      edititem = (R_gtk_edititem *) curfile->data;
      unlink(edititem->filename);
      curfile = g_list_next(curfile);
    }
*/
    if((tmpdir = getenv("R_SESSION_TMPDIR"))) {
	snprintf((char *)buf, 1024, "rm -rf %s", tmpdir);
	R_system((char *)buf);
    }

    /* close all the graphics devices */
    if(saveact != SA_SUICIDE) KillAllDevices();
    fpu_setup(FALSE);

    exit(status);
}

static void Rgnome_ShowMessage(char *s)
{
    GtkWidget *dialog;

    dialog = gnome_message_box_new(s,
				   GNOME_MESSAGE_BOX_WARNING,
				   GNOME_STOCK_BUTTON_OK,
				   NULL);
                           
    if(R_gtk_main_window != NULL)
	gnome_dialog_set_parent(GNOME_DIALOG(dialog), 
				GTK_WINDOW(R_gtk_main_window));
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);

    gnome_dialog_run_and_close(GNOME_DIALOG(dialog));    
}

	/*--- Initialization Code ---*/


static const struct poptOption popt_options[] = {
    { NULL, '\0', 0, NULL, 0, NULL, NULL }
};

/* for use in terminal-prefs.c */
void R_set_SaveAction(int sa) 
{
    SaveAction = sa;
}


int main(int ac, char **av)
{
    int i, ioff = 1, j;
    char *p, **avv;
    structRstart rstart;
    Rstart Rp = &rstart;
    struct stat sb;

    R_GUIType = "GNOME";
    ptr_R_Suicide = Rgnome_Suicide;
    ptr_R_ShowMessage = Rgnome_ShowMessage;
    ptr_R_ReadConsole = Rgnome_ReadConsole;
    ptr_R_WriteConsole = Rgnome_WriteConsole;
    ptr_R_ResetConsole = Rgnome_ResetConsole;
    ptr_R_FlushConsole = Rgnome_FlushConsole;
    ptr_R_ClearerrConsole = Rgnome_ClearerrConsole;
    ptr_R_Busy = Rgnome_Busy;
    ptr_R_CleanUp = Rgnome_CleanUp;
    ptr_R_ShowFiles = Rgnome_ShowFiles;
    ptr_R_ChooseFile = Rgnome_ChooseFile;
    ptr_R_loadhistory = Rgnome_loadhistory;
    ptr_R_savehistory = Rgnome_savehistory;

    R_timeout_handler = NULL;
    R_timeout_val = 0;

    R_GlobalContext = NULL; /* Make R_Suicide less messy... */

    if((R_Home = R_HomeDir()) == NULL)
	R_Suicide("R home directory is not defined");

    process_system_Renviron();

    R_setStartTime();
    R_DefParams(Rp);
    /* Store the command line arguments before they are processed
       by the R option handler. 
     */
    R_set_command_line_arguments(ac, av);


    /* Gnome startup preferences */
    gnomelib_init("R",
		  g_strdup_printf("%s.%s %s (%s-%s-%s)", R_MAJOR,
				  R_MINOR, R_STATUS, R_YEAR, R_MONTH,
				  R_DAY));
    /* The aim here seems to be to set user's defaults to R defaults */
    R_gnome_prefs_cmd_load(RestoreAction, SaveAction);

    Rp->RestoreAction = prefs_get_restoreact();
    Rp->SaveAction = prefs_get_saveact();

    /* command line params */
    R_common_command_line(&ac, av, Rp);

    /* remove GUI from command line args */
    for(i = 0, avv = av; i < ac; i++, avv++) {
	if(!strncmp(*avv, "--gui", 5) || !strncmp(*avv, "-g", 2)) {
	    if(!strncmp(*avv, "--gui", 5) && strlen(*avv) >= 7)
		p = &(*avv)[6];
	    else {
		if(i+1 < ac) {
		    avv++; p = *avv; ioff++;
		}
	    }
	    for(j = i; j < ac-ioff; j++)  av[j] = av[j + ioff];
	    ac -= ioff;
	    break;
	}
    }

    /* Initialise Gnome library */
    gnome_init("R",
	       g_strdup_printf("%s.%s %s (%s-%s-%s)", R_MAJOR, R_MINOR,
			       R_STATUS, R_YEAR, R_MONTH, R_DAY),
	       ac, av);

    /* Reset locale information */
#ifdef HAVE_LOCALE_H
    setlocale(LC_ALL, "C");
    setlocale(LC_CTYPE, "");/*- make ISO-latin1 etc. work for LOCALE users */
    setlocale(LC_COLLATE, "");/*- alphabetically sorting */
    setlocale(LC_TIME, "");/*- names and defaults for date-time formats */
#endif 

    /* Initialise libglade */
    glade_gnome_init();

    /* Gnome GUI preferences */
    R_gnome_prefs_gui_load();

    R_SetParams(Rp);
    if(!Rp->NoRenviron) {
	process_site_Renviron();
	process_user_Renviron();
    }

    R_Interactive = 1; /* want to be always interactive */
    if((R_Home = R_HomeDir()) == NULL) {
	R_Suicide("R home directory is not defined");
    }
    glade_interface_file = g_strdup_printf(GLADE_INTERFACE_FILE, R_Home);
    if(stat(glade_interface_file, &sb) == -1) {
	R_Suicide("GNOME interface file not found");
    }

/*
 *  Since users' expectations for save/no-save will differ, we decided
 *  that they should be forced to specify in the non-interactive case.
 *  But that does not occur!
    if (!R_Interactive && SaveAction != SA_SAVE && SaveAction != SA_NOSAVE)
	R_Suicide("you must specify `--save', `--no-save' or `--vanilla'");
 */

    /* create console */
    R_gtk_terminal_new();

    /* restore command history */
    R_setupHistory();
    if(R_RestoreHistory)
	gtk_console_restore_history(GTK_CONSOLE(R_gtk_terminal_text), 
				    R_HistoryFile, R_HistorySize, NULL);

    fpu_setup(TRUE);

    mainloop();
    /*++++++  in ../main/main.c */
    return 0;
}
