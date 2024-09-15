#include "gui.h"
#include "buffer.h"
#include "tab-page.h"
#include "server.h"
#include "client.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>

static const char* SHARE_RESPONSE_CANCEL = "cancel";
static const char* SHARE_RESPONSE_HOST = "host";
static const char* SHARE_RESPONSE_CONNECT = "connect";
// Minimum dimensions of the Window's main content (the TextView area).
static const unsigned int CONTENT_MIN_WIDTH = 400;
static const unsigned int CONTENT_MIN_HEIGHT = 200;

typedef enum {
    Server,
    Client,
    Off
} ConnectionState;
static ConnectionState connection_state = Off;

/// Handler for activating/deactivating the share feature. A GtkDialog will be crated on top of *window*.
static void share_toggle_click(GtkToggleButton* toggle, ShareClickParams* params) {
    if (gtk_toggle_button_get_active(toggle)) {
        // Deactivate toggle button. `share_enable_response()` should activate the toggle button if setup was successful.
        gtk_toggle_button_set_active(toggle, false);

        // ~~Freed by `share_enable_response()`.~~
        // Gives error when `g_free(builder), even though it should be freed accroding to https://docs.gtk.org/gtk4/ctor.Builder.new_from_resource.html
        GtkBuilder* builder = gtk_builder_new_from_resource("/me/Asder8215/TeXiT/share-dialog.ui");
        AdwMessageDialog* dialog = ADW_MESSAGE_DIALOG(gtk_builder_get_object(builder, "dialog"));
        ShareDialogEntries entries = share_dialog_entries(builder);
        // C moment :( Why must it be done like this
        // Freed by `share_enable_response()`.
        ShareEnableParams* enable_params = malloc(sizeof(ShareEnableParams));
        enable_params->window = params->window;
        enable_params->file_buttons = params->file_buttons;
        enable_params->label = params->label;
        enable_params->tabbar = params->tabbar;
        enable_params->toggle = toggle;
        enable_params->entries = entries;
        enable_params->toast_overlay = params->toast_overlay;
        enable_params->tab_view = params->tab_view;

        // Connect response callback
        gtk_window_set_transient_for(GTK_WINDOW(dialog), params->window);
        g_signal_connect(dialog, "response", G_CALLBACK(share_enable_response), enable_params);
        gtk_window_present(GTK_WINDOW(dialog));
    } else {
        if (connection_state == Server)
            /// TODO: does not turn off toggle when connected to client (at least when on the same machine)
            stop_server();
        else if (connection_state == Client)
            stop_client();
        connection_state = Off;
    }
}

/// Set up sharing connection (either hosting or connecting).
static void share_enable_response(AdwMessageDialog* dialog, const char* response, ShareEnableParams* params) {
    // The response_id ptr will be different than that of any of the set response_ids in `share_toggle_click()`, dont know why. Must use `strcmp()`.
    // strcmp() == 0 when strings are equal.
    if (strcmp(response, SHARE_RESPONSE_HOST) == 0) {
        // Will default to 1 if invalid string, but thats ok because 1 is not in valid port range.
        int port = atoi(gtk_editable_get_text(params->entries.host_port));

        printf("Starting host with port %d...\n", port);
        switch (start_server(port, params->tab_view, params->toggle)) {
            case Success:
                printf("Host started successfully.\n");
                connection_state = Server;
                break;
            case AlreadyStarted:
                fprintf(stderr, "Server is already running.\n");
                adw_toast_overlay_add_toast(params->toast_overlay, adw_toast_new("Server is already running"));
                break;
            case InvalidPort:
                fprintf(stderr, "Invalid Port number. Must be between %d and %d\n", PORT_MIN, PORT_MAX);
                adw_toast_overlay_add_toast(params->toast_overlay, adw_toast_new_format("Invalid Port number. Must be between %d and %d\n", PORT_MIN, PORT_MAX));
                break;
            case Other:
                fprintf(stderr, "Could not start hosting for the reason above.\n");
                adw_toast_overlay_add_toast(params->toast_overlay, adw_toast_new("Server not started. Check console output"));
                break;
        }
    } else if (strcmp(response, SHARE_RESPONSE_CONNECT) == 0) {
        const char* ip = gtk_editable_get_text(params->entries.connect_ip);
        int port = atoi(gtk_editable_get_text(params->entries.connect_port));

        printf("Connect to ip address %s with port %d\n", ip, port); 
        
        switch (start_client(ip, port, params->tab_view, params->file_buttons, params->toggle, params->label, params->window)) {
            case Success:
                printf("Client started successfully.\n");
                connection_state = Client;
                break;
            case AlreadyStarted:
                fprintf(stderr, "Client is already running.\n");
                adw_toast_overlay_add_toast(params->toast_overlay, adw_toast_new("Client is already running"));
                break;
            case InvalidPort:
                fprintf(stderr, "Invalid Port number. Must be between %d and %d\n", PORT_MIN, PORT_MAX);
                adw_toast_overlay_add_toast(params->toast_overlay, adw_toast_new("Invalid port. Must be between 1024 and 65535"));
                break;
            case Other:
                fprintf(stderr, "Could not start connecting for the reason above.\n");
                adw_toast_overlay_add_toast(params->toast_overlay, adw_toast_new("Client not started. Check console output"));
                break;
        }
    }

    free(params);
}


static void new_file_click(GtkButton* button, FileClickParams* params) {
    new_tab_page(params->tab_view, "Untitled", NULL);
    server_new_tab();
}

static void open_file_response(GtkFileDialog* dialog, GAsyncResult* result, FileClickParams* params) {
    GFile* file = gtk_file_dialog_open_finish(dialog, result, NULL);
    if (file != NULL) {
        const char* filePath = g_file_get_path(file);
        EditorBuffer* buffer = new_tab_page(params->tab_view, g_file_get_basename(file), filePath).buffer;

        char* content;
        GError* error;
        if (!g_file_load_contents(file, NULL, &content, NULL, NULL, &error)) {
            printf("Error opening file \"%s\": %s\n", filePath, error->message);
            free(error);
            exit(0);
        }
        
        // replace content of the buffer with file content.
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), content, -1);
        // Set to false because `gtk_text_buffer_set_text()` emmits signal "changed".
        editor_buffer_set_edited(buffer, false);
        // update title and set indicator icon to nothing
        adw_tab_page_set_title(editor_buffer_get_page(buffer), g_file_get_basename(file));
        adw_tab_page_set_indicator_icon(editor_buffer_get_page(buffer), NULL);
        // Server sends the new file to its clients
        server_new_tab();
        
        g_object_unref(file);
        free(content);
    }

    g_object_unref(dialog);
}
/// *params* is malloc-ed by `main_window()` (which essentially acts like `main()`) so it is allocated only once.
/// That same ptr should be freed only when the program terminates.
static void open_file_click(GtkButton* button, FileClickParams* params) {
    GtkFileDialog* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_modal(dialog, true);
    gtk_file_dialog_set_title(dialog, "Open File");
    gtk_file_dialog_open(dialog, params->window, NULL, (GAsyncReadyCallback)(open_file_response), params);
}

static void save_file_click(GtkButton* button, FileClickParams* params) {
    // could be NULL if no tabs are open
    Page page = get_active_page(params->tab_view);
    if (page.page == NULL) {
        adw_toast_overlay_add_toast(params->toast_overlay, adw_toast_new("No documents are open"));
        return;
    }
    editor_buffer_save(page.buffer, params->tab_view, params->window, false);
}


void main_window(AdwApplication *app) {
    GtkBuilder* builder = gtk_builder_new_from_resource("/me/Asder8215/TeXiT/main-window.ui");

    FileClickParams* file_click_params = malloc(sizeof(FileClickParams));
    ShareClickParams* share_click_params = malloc(sizeof(ShareClickParams));
    FileButtons* file_buttons = malloc(sizeof(FileButtons));
    MainMalloced* malloced = malloc(sizeof(MainMalloced));
    malloced->file_click_params = file_click_params;
    malloced->share_click_params = share_click_params;
    malloced->file_buttons = file_buttons;

    AdwApplicationWindow* window = ADW_APPLICATION_WINDOW(gtk_builder_get_object(builder, "main-window"));
    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));
    g_signal_connect(GTK_WIDGET(window), "close-request", G_CALLBACK(main_window_destroy), malloced);

    file_click_params->window = GTK_WINDOW(window);
    file_click_params->label = GTK_LABEL(gtk_builder_get_object(builder, "label"));
    file_click_params->tabbar = ADW_TAB_BAR(gtk_builder_get_object(builder, "tab-bar"));
    file_click_params->toast_overlay = ADW_TOAST_OVERLAY(gtk_builder_get_object(builder, "toast-overlay"));
    file_click_params->tab_view = ADW_TAB_VIEW(gtk_builder_get_object(builder, "tab-view"));
    g_signal_connect(file_click_params->tab_view, "close-page", G_CALLBACK(close_tab_page), GTK_WINDOW(window));

    file_buttons->file_new = GTK_BUTTON(gtk_builder_get_object(builder, "file-new"));
    file_buttons->file_open = GTK_BUTTON(gtk_builder_get_object(builder, "file-open"));
    file_buttons->file_save = GTK_BUTTON(gtk_builder_get_object(builder, "file-save"));
    g_signal_connect(file_buttons->file_new, "clicked", G_CALLBACK(new_file_click), file_click_params);
    g_signal_connect(file_buttons->file_open, "clicked", G_CALLBACK(open_file_click), file_click_params);
    g_signal_connect(file_buttons->file_save, "clicked", G_CALLBACK(save_file_click), file_click_params);

    share_click_params->window = file_click_params->window;
    share_click_params->label = file_click_params->label;
    share_click_params->tabbar = file_click_params->tabbar;
    share_click_params->toast_overlay = file_click_params->toast_overlay;
    share_click_params->tab_view = file_click_params->tab_view; 
    share_click_params->file_buttons = file_buttons;

    GtkToggleButton* share_toggle = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "share-toggle"));
    gtk_button_set_label(GTK_BUTTON(share_toggle), TOGGLE_LABEL_OFF);
    g_signal_connect(share_toggle, "clicked", G_CALLBACK(share_toggle_click), share_click_params);

    GtkButton* settings_btn = GTK_BUTTON(gtk_builder_get_object(builder, "settings-btn"));
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(settings_click), window);

    gtk_window_present(GTK_WINDOW(window));
}

gboolean main_window_destroy(AdwApplicationWindow* window, MainMalloced* params) {
    if (connection_state == Server)
        stop_server();
    else if (connection_state == Client)
        stop_client();
    free(params->file_click_params);
    free(params->share_click_params);
    free(params->file_buttons);
    free(params);
    return GDK_EVENT_PROPAGATE;
}


// /// Callback for when user presses Enter on the entry.
// /// Emtits the `response` signal with response_id "host".
// static void host_port_activate(GtkEditable* entry, AdwMessageDialog* dialog) {
//     // adw_message_dialog_response(dialog,SHARE_RESPONSE_HOST);
//     g_signal_emit_by_name(dialog, "response", "host");
// }
// /// Callback for when user presses Enter on the entry.
// /// Shifts focus to the `connect_port` entry.
// static void connect_ip_activate(GtkEditable* entry, GtkEditable* connect_port) {
//     gtk_widget_grab_focus(GTK_WIDGET(connect_port));
// }
// /// Callback for when user presses Enter on the entry.
// /// Emtits the `response` signal with response_id "connect".
// static void connect_port_activate(GtkEditable* entry, AdwMessageDialog* dialog) {
//     adw_message_dialog_response(dialog,SHARE_RESPONSE_CONNECT);
//     gtk_window_close(GTK_WINDOW(dialog));
// }
ShareDialogEntries share_dialog_entries(GtkBuilder* dialog_builder) {
    AdwMessageDialog* dialog = ADW_MESSAGE_DIALOG(gtk_builder_get_object(dialog_builder, "dialog"));
    GtkEditable* host_port = GTK_EDITABLE(gtk_builder_get_object(dialog_builder, "host-port"));
    GtkEditable* connect_ip = GTK_EDITABLE(gtk_builder_get_object(dialog_builder, "connect-ip"));
    GtkEditable* connect_port = GTK_EDITABLE(gtk_builder_get_object(dialog_builder, "connect-port"));
    // // Connect Entry callbacks for when user presses Enter
    // g_signal_connect(host_port, "entry-activated", G_CALLBACK(host_port_activate), dialog);
    // g_signal_connect(connect_ip, "entry-activated", G_CALLBACK(connect_ip_activate), connect_port);
    // g_signal_connect(connect_port, "entry-activated", G_CALLBACK(connect_port_activate), dialog);
    const char* default_port = g_strdup_printf("%d", DEFAULT_PORT);
    gtk_editable_set_text(host_port, default_port);
    gtk_editable_set_text(connect_port, default_port);
    g_free((void*)default_port);

    ShareDialogEntries r = {
        GTK_EDITABLE(host_port),
        GTK_EDITABLE(connect_ip),
        GTK_EDITABLE(connect_port),
    };
    return r;
}
