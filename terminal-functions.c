/*
 *  R : A Computer Langage for Statistical Data Analysis
 *  Copyright (C) 1998-2004   Lyndon Drake
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

#include <Rinterface.h> /* for onintr */
#include <R_ext/Print.h> /* for Rprintf */

#include "terminal.h"
#include "terminal-functions.h"

#include <signal.h>

static void file_open_ok(GtkWidget *widget, gpointer data);
static void file_saveas_ok(GtkWidget *widget, gpointer data);

/* Primitive functions:
   - run a string */

void R_gtk_terminal_run_initial() {
  /* delete any existing input */
  if(gtk_text_get_length(GTK_TEXT(R_gtk_terminal_text)) > GTK_CONSOLE(R_gtk_terminal_text)->input_start_index) {
    gtk_editable_delete_text(GTK_EDITABLE(R_gtk_terminal_text), GTK_CONSOLE(R_gtk_terminal_text)->input_start_index, gtk_text_get_length(GTK_TEXT(R_gtk_terminal_text)));
  }
}

void R_gtk_terminal_run_partial(gchar *code) {
  gint insert_pos;

  /* where to insert the command */
  insert_pos = gtk_text_get_length(GTK_TEXT(R_gtk_terminal_text));

  /* insert the command */
  gtk_editable_insert_text(GTK_EDITABLE(R_gtk_terminal_text), code, strlen(code), &insert_pos);
}

void R_gtk_terminal_run_final(gchar *code) {
  gint insert_pos;

  /* where to insert the command */
  insert_pos = gtk_text_get_length(GTK_TEXT(R_gtk_terminal_text));

  /* insert the command */
  gtk_editable_insert_text(GTK_EDITABLE(R_gtk_terminal_text), code, strlen(code), &insert_pos);

  if(!strchr(code, '\n')) {
    insert_pos = gtk_text_get_length(GTK_TEXT(R_gtk_terminal_text));
    gtk_editable_insert_text(GTK_EDITABLE(R_gtk_terminal_text), "\n", 1, &insert_pos);
  }

  /* signal the text box */
  gtk_signal_emit_by_name(GTK_OBJECT(R_gtk_terminal_text), "console_line_ready");
}

void R_gtk_terminal_run(gchar *code)
{
  R_gtk_terminal_run_initial();
  R_gtk_terminal_run_final(code);
}



/* Higher level functions */

static GtkWidget *R_gtk_os_file = NULL;
static gint R_gtk_os_signal_ok;


void R_gtk_terminal_interrupt()
{
  onintr();
}

void R_gtk_terminal_quit()
{
  R_gtk_terminal_run("quit()\n");
}

static void file_open_ok(GtkWidget *widget, gpointer data)
{
  char *name;

  R_gtk_terminal_run("\n");

  name = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
  R_RestoreGlobalEnvFromFile(name, TRUE);
  Rprintf("Previously saved workspace restored\n");

  gtk_widget_hide(GTK_WIDGET(data));
}

void R_gtk_terminal_file_open(GtkWidget *widget, gpointer data)
{
  if(!R_gtk_os_file) {
    R_gtk_os_file = gtk_file_selection_new("Open R file");

    gtk_window_set_transient_for(GTK_WINDOW(R_gtk_os_file), GTK_WINDOW(R_gtk_main_window));
    gtk_window_set_modal(GTK_WINDOW(R_gtk_os_file), TRUE);

    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(R_gtk_os_file)->cancel_button),
			      "clicked",
			      (GtkSignalFunc) gtk_widget_hide,
			      GTK_OBJECT(R_gtk_os_file));
  }
  else {
    gtk_window_set_title(&GTK_FILE_SELECTION(R_gtk_os_file)->window, "Open R file");
    gtk_signal_disconnect(GTK_OBJECT(GTK_FILE_SELECTION(R_gtk_os_file)->ok_button),
			  R_gtk_os_signal_ok);
  }

  R_gtk_os_signal_ok = gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(R_gtk_os_file)->ok_button),
		     "clicked",
		     (GtkSignalFunc) file_open_ok,
		     GTK_OBJECT(R_gtk_os_file));

  gtk_widget_show(R_gtk_os_file);
}

void R_gtk_terminal_file_save(GtkWidget *widget, gpointer data)
{
}

static void file_saveas_ok(GtkWidget *widget, gpointer data)
{
    char *name;
    R_gtk_terminal_run("\n");
    name = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));
    R_SaveGlobalEnvToFile(name);
    Rprintf("Workspace saved\n");
    gtk_widget_hide(GTK_WIDGET(data));
}

void R_gtk_terminal_file_saveas(GtkWidget *widget, gpointer data)
{
  if(!R_gtk_os_file) {
    R_gtk_os_file = gtk_file_selection_new("Save R file");

    gtk_window_set_transient_for(GTK_WINDOW(R_gtk_os_file), GTK_WINDOW(R_gtk_main_window));
    gtk_window_set_modal(GTK_WINDOW(R_gtk_os_file), TRUE);

    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(R_gtk_os_file)->cancel_button),
			      "clicked",
			      (GtkSignalFunc) gtk_widget_destroy,
			      GTK_OBJECT(R_gtk_os_file));
  }
  else {
    gtk_window_set_title(GTK_WINDOW(R_gtk_os_file), "Save R file");
    gtk_signal_disconnect(GTK_OBJECT(GTK_FILE_SELECTION(R_gtk_os_file)->ok_button),
			  R_gtk_os_signal_ok);
  }

  R_gtk_os_signal_ok = gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(R_gtk_os_file)->ok_button),
		     "clicked",
		     (GtkSignalFunc) file_saveas_ok,
		     GTK_OBJECT(R_gtk_os_file));

  gtk_widget_show(R_gtk_os_file);
}

void R_gtk_terminal_device_activate(gint devnum) {
}
