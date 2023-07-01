#include "buffer.h"
#include "gui.h"
#include "tab-page.h"
#include <stdio.h>

Page new_tab_page(AdwTabView* tab_view, const char* title, const char* filePath) {
    Widget scroller, text_view;
    Page rtrn;
    
    scroller = gtk_scrolled_window_new();
    rtrn.page = adw_tab_view_append(tab_view, scroller);
    rtrn.buffer = editor_buffer_new(filePath, rtrn.page);
    text_view = gtk_text_view_new_with_buffer(GTK_TEXT_BUFFER(rtrn.buffer));
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), text_view);

    adw_tab_page_set_title(rtrn.page, title);
    adw_tab_page_set_icon(rtrn.page, g_themed_icon_new("text-x-generic-symbolic"));

	return rtrn;
}
Page get_active_page(AdwTabView* tab_view) {
    Page rtrn;
    rtrn.page = adw_tab_view_get_selected_page(tab_view);

    if (rtrn.page != NULL)
        rtrn.buffer = EDITOR_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(
            gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(
                adw_tab_page_get_child(rtrn.page)
            ))
        )));
    else
        rtrn.buffer = NULL;
    
	return rtrn;
}

/// Handles response receive from the close tab page message dialog.
static void close_unsaved_tab_response(AdwMessageDialog* dialog, GAsyncResult* result, FileClickParams* params) {  
    const char* response = adw_message_dialog_choose_finish(dialog, result);
    Page curr_page = get_active_page(params->tab_view);
    
    // save response
    if (strcmp(response, "save") == 0)
        // calls tab_view.close_page_finish(true) if file was successfully saved
        editor_buffer_save(curr_page.buffer, params->tab_view, params->window, true);
    // close response
    else if (strcmp(response, "close") == 0) 
        adw_tab_view_close_page_finish(params->tab_view, ADW_TAB_PAGE(curr_page.page), true);
    // cancel response
    else if (strcmp(response, "cancel") == 0)
        adw_tab_view_close_page_finish(params->tab_view, ADW_TAB_PAGE(curr_page.page), false);

    if (adw_tab_view_get_n_pages(params->tab_view) == 0)
        gtk_widget_set_visible(GTK_WIDGET(params->tab_view), false);

    free(params);
}

// TODO: Come back to this later to find a more efficient way of detecting
// whether a file has been edited or not.
/// Handler for closing a tab page.
gboolean close_tab_page(AdwTabView* tab_view, AdwTabPage* page, GtkWindow* window) {
    Page curr_page = get_active_page(tab_view);
    
    const char* file_path = editor_buffer_get_file_path(curr_page.buffer);
    
    GFile* file;
    if(file_path != NULL){
        file = g_file_new_for_path(file_path);
    }
    else{
        file = NULL;
    }
    const char* file_name;
    
    // file_name utilized for the close tab message dialog
    if(file != NULL){
        file_name = g_file_get_basename(file);
    }
    else{
        file_name = "Untitled";
    }
    
    // utilizes the edited var boolean in editor buffer to check if the buffer is changed or not.
    int is_buffer_edited = 0;
    if(editor_buffer_get_edited(curr_page.buffer)){
        is_buffer_edited = 1;
    }
    
    // Prompt user with if they want to cancel closing the tab, closing the
    // tab even with unsaved changes, or save the content in the tab.
    if(is_buffer_edited){
        AdwMessageDialog* dialog = ADW_MESSAGE_DIALOG(adw_message_dialog_new(GTK_WINDOW(window), "Save File?", NULL));

        adw_message_dialog_format_body(ADW_MESSAGE_DIALOG(dialog), "There are unsaved changes in %s.",
                                       file_name);

        adw_message_dialog_add_responses(ADW_MESSAGE_DIALOG(dialog), "cancel", "Cancel", "close", "Close", 
                                         "save", "Save", NULL);

        adw_message_dialog_set_response_appearance(ADW_MESSAGE_DIALOG(dialog), "save",
                                                   ADW_RESPONSE_SUGGESTED);

        adw_message_dialog_set_response_appearance(ADW_MESSAGE_DIALOG(dialog), "close",
                                                   ADW_RESPONSE_DESTRUCTIVE);
        
        
        adw_message_dialog_set_default_response(ADW_MESSAGE_DIALOG(dialog), "cancel");
        adw_message_dialog_set_close_response(ADW_MESSAGE_DIALOG(dialog), "cancel");

        FileClickParams* params = malloc(sizeof(FileClickParams));
        params->window = GTK_WINDOW(dialog);
        params->tab_view = ADW_TAB_VIEW(tab_view);
        params->toast_overlay = NULL;

        gtk_window_set_modal(GTK_WINDOW(dialog), true);
        gtk_window_set_transient_for(GTK_WINDOW(dialog), window);
        gtk_window_set_resizable(GTK_WINDOW(dialog), false);
        
        // Instead of using g_signal_connect and gtk_window_present
        // This method allows for an async call on the desired function.
        adw_message_dialog_choose(ADW_MESSAGE_DIALOG(dialog), NULL, (GAsyncReadyCallback) close_unsaved_tab_response, params);
    }
    // Close the tab if nothing's changed.
    else{
        adw_tab_view_close_page_finish(tab_view, curr_page.page, true);
        if (adw_tab_view_get_n_pages(tab_view) == 0)
            gtk_widget_set_visible(GTK_WIDGET(tab_view), false);
    }

    // This return value prevents default handlers from being called on
    // ^Took me forever to figure this out.
    return GDK_EVENT_STOP;
}

