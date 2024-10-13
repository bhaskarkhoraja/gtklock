// gtklock
// Copyright (c) 2022 Kenny Levinsen, Jovan Lanik, Erik Reider, Melih Darcan, Bhaskar Khoraja

// Window functions

#include <time.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gtk-session-lock.h>

#include "util.h"
#include "window.h"
#include "gtklock.h"
#include "auth.h"
#include "module.h"

extern struct GtkLock *gtklock;

struct Window *window_by_widget(GtkWidget *window) {
	for(guint idx = 0; idx < gtklock->windows->len; idx++) {
		struct Window *ctx = g_array_index(gtklock->windows, struct Window *, idx);
		if(ctx->window == window) return ctx;
	}
	return NULL;
}

struct Window *window_by_monitor(GdkMonitor *monitor) {
	for(guint idx = 0; idx < gtklock->windows->len; idx++) {
		struct Window *ctx = g_array_index(gtklock->windows, struct Window *, idx);
		if(ctx->monitor == monitor) return ctx;
	}
	return NULL;
}

struct Window *window_last_active(void) {
	GtkWindow *window = gtk_application_get_active_window(gtklock->app);
	if(window) return window_by_widget(GTK_WIDGET(window));
	return NULL;
}

void window_update_clock(struct Window *ctx) {
	gtk_label_set_text(GTK_LABEL(ctx->clock_label), gtklock->time);
}

void window_update_date(struct Window *ctx) {
	gtk_label_set_text(GTK_LABEL(ctx->date_label), gtklock->date);
}

void window_update_username(struct Window *ctx) {
	gtk_label_set_text(GTK_LABEL(ctx->user_field), gtklock->username);
}

static void window_setup_messages(struct Window *ctx);

static void window_setup_messages(struct Window *ctx) {
	if(ctx->message_label != NULL) {
    gtk_label_set_text(GTK_LABEL(ctx->message_label), "");
	}

	for(guint idx = 0; idx < gtklock->errors->len; idx++) {
		char *err = g_array_index(gtklock->errors, char *, idx);
    gtk_label_set_text(GTK_LABEL(ctx->message_label), err);
	}
	for(guint idx = 0; idx < gtklock->messages->len; idx++) {
		char *msg = g_array_index(gtklock->messages, char *, idx);
    gtk_label_set_text(GTK_LABEL(ctx->message_label), msg);
	}

}

static void window_set_busy(struct Window *ctx, gboolean busy) {
	if(busy) g_application_mark_busy(G_APPLICATION(gtklock->app));
	else g_application_unmark_busy(G_APPLICATION(gtklock->app));

	GdkCursor *cursor = gdk_cursor_new_from_name(gtk_widget_get_display(ctx->window), busy ? "wait" : "default");
	gdk_window_set_cursor(gtk_widget_get_window(ctx->window), cursor);
	g_object_unref(cursor);

	// gtk_widget_set_sensitive(ctx->unlock_button, !busy);
	gtk_widget_set_sensitive(ctx->input_field, !busy);
}

static gboolean window_pw_failure(gpointer data) {
	struct Window *ctx = data;
	window_set_busy(ctx, FALSE);
	gtk_entry_set_text(GTK_ENTRY(ctx->input_field), "");
	gtk_entry_grab_focus_without_selecting(GTK_ENTRY(ctx->input_field));
	gtk_label_set_text(GTK_LABEL(ctx->error_label), _("Login failed"));
	return G_SOURCE_REMOVE;
}

static gboolean window_pw_message(gpointer data) {
	window_setup_messages((struct Window *)data);
	return G_SOURCE_REMOVE;
}

static gpointer window_pw_wait(gpointer data) {
	struct Window *ctx = data;
	const char *password = gtk_entry_get_text((GtkEntry*)ctx->input_field);
	while(TRUE) {
		enum pwcheck ret = auth_pw_check(password);
		switch(ret) {
			case PW_FAILURE:
				g_main_context_invoke(NULL, window_pw_failure, ctx);
				return NULL;
			case PW_SUCCESS:
				g_application_quit(G_APPLICATION(gtklock->app));
				return NULL;
			case PW_ERROR:
				{
					char *err = auth_get_error();
					g_array_append_val(gtklock->errors, err);
					g_main_context_invoke(NULL, window_pw_message, ctx);
				}
				break;
			case PW_MESSAGE:
				{
					char *msg = auth_get_message();
					g_array_append_val(gtklock->messages, msg);
					g_main_context_invoke(NULL, window_pw_message, ctx);
				}
				break;
			case PW_WAIT:
				break;
		}
	}

}

void window_pw_check(GtkWidget *widget, gpointer data) {
	struct Window *ctx = data;
	window_set_busy(ctx, TRUE);
	gtk_label_set_text(GTK_LABEL(ctx->error_label), NULL);
	g_thread_new(NULL, window_pw_wait, ctx);
}

void input_field_changed(GtkWidget *widget, gpointer data) {
	struct Window *ctx = data;
	gtk_label_set_text(GTK_LABEL(ctx->error_label), NULL);
}

static void window_pw_set_vis(GtkEntry* entry, gboolean visibility) {
	const char *icon = visibility ? "view-conceal-symbolic" : "view-reveal-symbolic";
	gtk_entry_set_icon_from_icon_name(entry, GTK_ENTRY_ICON_SECONDARY, icon);
	gtk_entry_set_visibility(entry, visibility);
}

void window_pw_toggle_vis(GtkEntry* entry, GtkEntryIconPosition icon_pos) {
	if(icon_pos != GTK_ENTRY_ICON_SECONDARY) return;
	gboolean visibility = gtk_entry_get_visibility(entry);
	window_pw_set_vis(entry, !visibility);
}

static void window_destroy_notify(GtkWidget *widget, gpointer data) {
	struct Window *win = window_by_widget(widget);
	module_on_window_destroy(gtklock, win);
	gtk_widget_destroy(widget);
	gtklock_remove_window(gtklock, win);
}

void window_swap_focus(struct Window *win, struct Window *old) {

	GtkStyleContext *win_context = gtk_widget_get_style_context(win->window);
	gtk_style_context_add_class(win_context, "focused");

	if(old != NULL && old != win) {

		GtkStyleContext *old_context = gtk_widget_get_style_context(old->window);
		gtk_style_context_remove_class(old_context, "focused");

		if(old->input_field != NULL && win->input_field != NULL) {
			// Get previous cursor position
			gint cursor_pos = 0;
			g_object_get(GTK_ENTRY(old->input_field), "cursor-position", &cursor_pos, NULL);

			// Move content
			gtk_entry_set_text(GTK_ENTRY(win->input_field), gtk_entry_get_text(GTK_ENTRY(old->input_field)));
			gtk_entry_set_text(GTK_ENTRY(old->input_field), "");

			// Update new cursor position
			g_signal_emit_by_name(GTK_ENTRY(win->input_field), "move-cursor", GTK_MOVEMENT_BUFFER_ENDS, -1, FALSE);
			g_signal_emit_by_name(GTK_ENTRY(win->input_field), "move-cursor", GTK_MOVEMENT_LOGICAL_POSITIONS, cursor_pos, FALSE);

			// Copy pw visibility
			window_pw_set_vis(GTK_ENTRY(win->input_field), gtk_entry_get_visibility(GTK_ENTRY(old->input_field)));
		}
	}

	gtk_entry_grab_focus_without_selecting(GTK_ENTRY(win->input_field));
}

void window_idle_hide(struct Window *ctx) {
	GtkStyleContext *context = gtk_widget_get_style_context(ctx->window);
	gtk_style_context_add_class(context, "hidden");
	GdkCursor *cursor = gdk_cursor_new_for_display(gtk_widget_get_display(ctx->window), GDK_BLANK_CURSOR);
	gdk_window_set_cursor(gtk_widget_get_window(ctx->window), cursor);
	g_object_unref(cursor);
}

void window_idle_show(struct Window *ctx) {
	GtkStyleContext *context = gtk_widget_get_style_context(ctx->window);
	gtk_style_context_remove_class(context, "hidden");
	if(ctx == gtklock->focused_window) {
		gtk_entry_grab_focus_without_selecting(GTK_ENTRY(ctx->input_field));
	}
	GdkCursor *cursor = gdk_cursor_new_from_name(gtk_widget_get_display(ctx->window), "default");
	gdk_window_set_cursor(gtk_widget_get_window(ctx->window), cursor);
	g_object_unref(cursor);
}

static gboolean window_idle_key(GtkWidget *self, GdkEventKey event, gpointer user_data) {
	gtklock_idle_show(gtklock);
	return FALSE;
}
static gboolean window_idle_motion(GtkWidget *self, GdkEventMotion event, gpointer user_data) {
	gtklock_idle_show(gtklock);
	return FALSE;
}

static void window_caps_state_changed(GdkKeymap *self, gpointer user_data) {
	struct Window *w = gtklock->focused_window;
	if(!w || !w->warning_label) return;

	if(gdk_keymap_get_caps_lock_state(self)) gtk_label_set_text(GTK_LABEL(w->warning_label), _("Caps Lock is on"));
	else gtk_label_set_text(GTK_LABEL(w->warning_label), "");
}

static gboolean entry_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	if(event->button != 1) return TRUE;
	return FALSE;
}

static gboolean window_enter_notify(GtkWidget *widget, gpointer data) {
	struct Window *win = window_by_widget(widget);
	gtk_entry_grab_focus_without_selecting(GTK_ENTRY(win->input_field));
	gtklock_focus_window(gtklock, win);
	return FALSE;
}

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

struct Window *create_window(GdkMonitor *monitor) {
	struct Window *w = g_malloc0(sizeof(struct Window) + gtklock->modules->len * sizeof(void *));
	if(!w) report_error_and_exit("Failed allocation");

	g_array_append_val(gtklock->windows, w);
	w->monitor = monitor;
	w->window = gtk_application_window_new(gtklock->app);

	g_signal_connect(w->window, "destroy", G_CALLBACK(window_destroy_notify), NULL);
	if(gtklock->follow_focus)
		g_signal_connect(w->window, "enter-notify-event", G_CALLBACK(window_enter_notify), NULL);
	if(gtklock->use_idle_hide || gtklock->hidden) {
		gtk_widget_add_events(w->window, GDK_POINTER_MOTION_MASK);
		g_signal_connect(w->window, "key-press-event", G_CALLBACK(window_idle_key), NULL);
		g_signal_connect(w->window, "motion-notify-event", G_CALLBACK(window_idle_motion), NULL);
	}

	GdkDisplay *display = gtk_widget_get_display(w->window);

	/*
		This code uses a deprecated function and assumes one GDK screen...
		However there isn't really a good way to do this in GTK3 currently.
		Related issue: https://gitlab.gnome.org/GNOME/gtk/-/issues/4982
	*/
	char *name = NULL;
	GdkScreen *screen = gtk_widget_get_screen(w->window);
	for(int i = 0; i < gdk_display_get_n_monitors(display); i++) {
		GdkMonitor *monitor = gdk_display_get_monitor(display, i);
		if(monitor != w->monitor) continue;
		name = gdk_screen_get_monitor_plug_name(screen, i);
	}

	GdkKeymap *keymap = gdk_keymap_get_for_display(display);
	g_signal_connect(keymap, "state-changed", G_CALLBACK(window_caps_state_changed), NULL);

	if(name) {
		gtk_widget_set_name(w->window, name);
		g_free(name);
	}
	gtk_window_set_title(GTK_WINDOW(w->window), "Lockscreen");
	gtk_window_set_decorated(GTK_WINDOW(w->window), FALSE);
	gtk_widget_realize(w->window);
	gtk_session_lock_lock_new_surface(gtklock->lock, GTK_WINDOW(w->window), monitor);

	w->overlay = gtk_overlay_new();
	gtk_container_add(GTK_CONTAINER(w->window), w->overlay);
	
	GtkBuilder *builder = NULL;
	if(gtklock->layout_path) builder = gtk_builder_new_from_file(gtklock->layout_path);
	else builder = gtk_builder_new_from_resource("/gtklock/gtklock.ui");
	
	gtk_builder_connect_signals(builder, w);

	w->window_box = GTK_WIDGET(gtk_builder_get_object(builder, "window-box"));
	gtk_container_add(GTK_CONTAINER(w->overlay), w->window_box);

	w->body_grid = GTK_WIDGET(gtk_builder_get_object(builder, "body-grid"));
	w->input_label = GTK_WIDGET(gtk_builder_get_object(builder, "input-label"));

	w->input_field = GTK_WIDGET(gtk_builder_get_object(builder, "input-field"));
	g_signal_connect(w->input_field, "button-press-event", G_CALLBACK(entry_button_press), NULL);

	w->user_field = GTK_WIDGET(gtk_builder_get_object(builder, "user-field"));
	window_update_username(w);

	w->message_label = GTK_WIDGET(gtk_builder_get_object(builder, "message-label"));
	w->unlock_button = GTK_WIDGET(gtk_builder_get_object(builder, "unlock-button"));
	w->error_label = GTK_WIDGET(gtk_builder_get_object(builder, "error-label"));
	w->warning_label = GTK_WIDGET(gtk_builder_get_object(builder, "warning-label"));

	w->clock_label = GTK_WIDGET(gtk_builder_get_object(builder, "clock-label"));
	window_update_clock(w);

	w->date_label = GTK_WIDGET(gtk_builder_get_object(builder, "date-label"));
	window_update_date(w);

	if(gtklock->hidden) window_idle_hide(w);
	module_on_window_create(gtklock, w);
	gtk_widget_show_all(w->window);

	g_object_unref(builder);
	return w;
}

