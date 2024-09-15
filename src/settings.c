#include "settings.h"

void settings_click(GtkButton* settings_btn, GtkWindow* window){
    GtkBuilder* settings_builder = gtk_builder_new_from_resource("/me/Asder8215/TeXiT/settings.ui");
    AdwPreferencesWindow* prefs_window = ADW_PREFERENCES_WINDOW(gtk_builder_get_object(settings_builder, "setting-dialog"));
    gtk_window_set_transient_for(GTK_WINDOW(prefs_window), window);
    gtk_window_present(GTK_WINDOW(prefs_window));
}

