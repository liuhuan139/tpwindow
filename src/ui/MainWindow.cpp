#include "MainWindow.hpp"
#include "HierarchyWindow.hpp"
#include "../core/AdbManager.hpp"
#include "../core/CacheManager.hpp"
#include "../core/XmlParser.hpp"
#include <memory>
#include <string>

G_DEFINE_TYPE(MainWindow, main_window, GTK_TYPE_APPLICATION_WINDOW)

static void delete_ui_node(gpointer p) {
    delete static_cast<UiNode*>(p);
}

static void show_hierarchy_window(MainWindow* window, UiNode* root, int rotation, bool take_ownership, const char* cached_file, const char* screenshot_file) {
    GDestroyNotify destroy = take_ownership ? delete_ui_node : nullptr;
    if (window->hierarchy_window) {
        gtk_window_destroy(GTK_WINDOW(window->hierarchy_window));
    }
    window->hierarchy_window = GTK_WIDGET(hierarchy_window_new(GTK_WINDOW(window), root, destroy, rotation, cached_file, screenshot_file));
    gtk_window_present(GTK_WINDOW(window->hierarchy_window));
}

static void update_button_states(MainWindow* window) {
    gboolean can_click = window->adb_connected || window->has_cached_data;
    gtk_widget_set_sensitive(window->startal_button, can_click);
    gtk_widget_set_visible(window->refetch_button, window->has_cached_data);
}

static void on_check_clicked(GtkWidget* button, gpointer user_data) {
    MainWindow* window = MAIN_WINDOW(user_data);
    AdbManager adb;

    window->adb_connected = adb.is_device_connected();

    if (window->adb_connected) {
        gtk_label_set_text(GTK_LABEL(window->adb_status_label), "✓ ADB Connected");
        gtk_widget_add_css_class(window->adb_status_label, "success");
        gtk_widget_remove_css_class(window->adb_status_label, "error");
    } else {
        gtk_label_set_text(GTK_LABEL(window->adb_status_label), "✗ ADB Not Connected");
        gtk_widget_add_css_class(window->adb_status_label, "error");
        gtk_widget_remove_css_class(window->adb_status_label, "success");
    }

    update_button_states(window);
}

typedef struct {
    MainWindow* window;
    gboolean force_refetch;
} DumpThreadData;

typedef struct {
    MainWindow* window;
    gboolean success;
    UiNode* root;
    int rotation;
    char* local_file;
    char* screenshot_file;
    char* error_message;
    char*status_message;
    gboolean has_cached_data;
} DumpResultData;

static gboolean dump_finished_on_main(gpointer user_data) {
    DumpResultData* result = static_cast<DumpResultData*>(user_data);
    MainWindow* window = result->window;

    gtk_spinner_stop(GTK_SPINNER(window->spinner));
    gtk_widget_set_sensitive(window->startal_button, TRUE);
    gtk_widget_set_sensitive(window->refetch_button, TRUE);

    if (result->status_message) {
        gtk_label_set_text(GTK_LABEL(window->status_label), result->status_message);
    }

    if (result->success) {
        if (result->has_cached_data) {
            window->has_cached_data = TRUE;
            g_free(window->cached_file);
            window->cached_file = result->local_file;
            result->local_file = nullptr;
            g_free(window->screenshot_file);
            window->screenshot_file = result->screenshot_file;
            result->screenshot_file = nullptr;
            update_button_states(window);
        }
        if (result->root) {
            show_hierarchy_window(window, result->root, result->rotation, true,
                                  result->local_file ? result->local_file : window->cached_file,
                                  result->screenshot_file ? result->screenshot_file : window->screenshot_file);
        }
    } else {
        if (result->error_message) {
            gtk_label_set_text(GTK_LABEL(window->error_label), result->error_message);
            gtk_widget_set_visible(window->error_label, true);
        }
    }

    g_free(result->status_message);
    g_free(result->error_message);
    g_free(result->local_file);
    g_free(result->screenshot_file);
    g_free(result);
    return G_SOURCE_REMOVE;
}

static gpointer dump_thread_func(gpointer user_data) {
    DumpThreadData* thread_data = static_cast<DumpThreadData*>(user_data);
    MainWindow* window = thread_data->window;
    gboolean force_refetch = thread_data->force_refetch;

    DumpResultData* result = g_new0(DumpResultData, 1);
    result->window = window;
    result->success = false;
    result->root = nullptr;
    result->rotation = 0;
    result->local_file = nullptr;
    result->screenshot_file = nullptr;
    result->error_message = nullptr;
    result->status_message = nullptr;
    result->has_cached_data = false;

    AdbManager adb;
    CacheManager cache(adb.get_cache_directory());

    if (!force_refetch && window->has_cached_data && window->cached_file) {
        XmlParser parser;
        auto root = parser.parse_file(window->cached_file);
        if (root) {
            result->status_message = g_strdup("Showing cached UI hierarchy");
            result->root = root.release();
            result->rotation = parser.get_rotation();
            result->local_file = g_strdup(window->cached_file);
            result->screenshot_file = window->screenshot_file ? g_strdup(window->screenshot_file) : nullptr;
            result->success = true;
            g_free(thread_data);
            g_idle_add(dump_finished_on_main, result);
            return nullptr;
        }
    }

    if (!adb.is_device_connected()) {
        result->status_message = g_strdup("Failed to connect");
        result->error_message = g_strdup(adb.get_error_message().c_str());
        result->success = false;
        g_free(thread_data);
        g_idle_add(dump_finished_on_main, result);
        return nullptr;
    }

    if (!adb.ensure_temp_directory()) {
        result->status_message = g_strdup("Failed");
        result->error_message = g_strdup(adb.get_error_message().c_str());
        result->success = false;
        g_free(thread_data);
        g_idle_add(dump_finished_on_main, result);
        return nullptr;
    }

    std::string screenshot_remote = "/sdcard/screen.png";
    char* local_screenshot = nullptr;
    if (!adb.take_screenshot(screenshot_remote)) {
        // Continue without screenshot
    } else {
        std::string screenshot_local = cache.generate_new_cache_filename();
        size_t last_dot = screenshot_local.find_last_of('.');
        if (last_dot != std::string::npos) {
            screenshot_local = screenshot_local.substr(0, last_dot) + ".png";
        }
        if (adb.pull_screenshot(screenshot_remote, screenshot_local)) {
            local_screenshot = g_strdup(screenshot_local.c_str());
        }
    }

    if (!adb.dump_ui_hierarchy()) {
        result->status_message = g_strdup("Dump failed");
        result->error_message = g_strdup(adb.get_error_message().c_str());
        g_free(local_screenshot);
        result->success = false;
        g_free(thread_data);
        g_idle_add(dump_finished_on_main, result);
        return nullptr;
    }

    std::string local_file = cache.generate_new_cache_filename();
    if (!adb.pull_dump_file(local_file)) {
        result->status_message = g_strdup("Pull failed");
        result->error_message = g_strdup(adb.get_error_message().c_str());
        g_free(local_screenshot);
        result->success = false;
        g_free(thread_data);
        g_idle_add(dump_finished_on_main, result);
        return nullptr;
    }

    XmlParser parser;
    auto root = parser.parse_file(local_file);
    if (!root) {
        result->status_message = g_strdup("Parse failed");
        result->error_message = g_strdup(parser.get_error_message().c_str());
        g_free(local_screenshot);
        result->success = false;
        g_free(thread_data);
        g_idle_add(dump_finished_on_main, result);
        return nullptr;
    }

    result->has_cached_data = TRUE;
    result->local_file = g_strdup(local_file.c_str());
    result->screenshot_file = local_screenshot;
    result->root = root.release();
    result->rotation = parser.get_rotation();
    result->status_message = g_strdup("Ready");
    result->success = true;

    g_free(thread_data);
    g_idle_add(dump_finished_on_main, result);
    return nullptr;
}

static void perform_dump_async(MainWindow* window, gboolean force_refetch) {
    gtk_widget_set_sensitive(window->startal_button, FALSE);
    gtk_widget_set_sensitive(window->refetch_button, FALSE);
    gtk_spinner_start(GTK_SPINNER(window->spinner));
    gtk_widget_set_visible(window->error_label, FALSE);
    gtk_label_set_text(GTK_LABEL(window->status_label), "Checking adb connection...");

    DumpThreadData* thread_data = g_new0(DumpThreadData, 1);
    thread_data->window = window;
    thread_data->force_refetch = force_refetch;

    g_thread_new("dump-thread", dump_thread_func, thread_data);
}

static void on_startal_clicked(GtkWidget* button, gpointer user_data) {
    MainWindow* window = MAIN_WINDOW(user_data);
    perform_dump_async(window, FALSE);
}

static void on_refetch_clicked(GtkWidget* button, gpointer user_data) {
    MainWindow* window = MAIN_WINDOW(user_data);
    perform_dump_async(window, TRUE);
}

static void on_cleanup_activate(GSimpleAction* action, GVariant* parameter, gpointer user_data) {
    MainWindow* window = MAIN_WINDOW(user_data);
    AdbManager adb;
    CacheManager cache(adb.get_cache_directory());

    // Clear all cached files
    cache.clear_all_cache();

    // Reset window state
    window->has_cached_data = FALSE;
    g_free(window->cached_file);
    window->cached_file = nullptr;
    g_free(window->screenshot_file);
    window->screenshot_file = nullptr;

    gtk_label_set_text(GTK_LABEL(window->status_label), "Cache cleared");
    gtk_label_set_text(GTK_LABEL(window->adb_status_label), "ADB Status: Unknown");
    gtk_widget_remove_css_class(window->adb_status_label, "success");
    gtk_widget_remove_css_class(window->adb_status_label, "error");
    update_button_states(window);
}

static void on_quit_activate(GSimpleAction* action, GVariant* parameter, gpointer user_data) {
    GtkWindow* window = GTK_WINDOW(user_data);
    gtk_window_close(window);
}

static void on_about_activate(GSimpleAction* action, GVariant* parameter, gpointer user_data) {
    GtkWindow* parent = GTK_WINDOW(user_data);
    GtkWidget* dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "About TopWindow");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 450, 350);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    gtk_window_set_child(GTK_WINDOW(dialog), vbox);

    // Title
    GtkWidget* title_label = gtk_label_new("TopWindow");
    PangoAttrList* title_attrs = pango_attr_list_new();
    pango_attr_list_insert(title_attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(title_attrs, pango_attr_size_new(20 * PANGO_SCALE));
    gtk_label_set_attributes(GTK_LABEL(title_label), title_attrs);
    pango_attr_list_unref(title_attrs);
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(vbox), title_label);

    // Version
    GtkWidget* version_label = gtk_label_new("Version 0.1.0");
    gtk_widget_set_halign(version_label, GTK_ALIGN_START);
    gtk_widget_add_css_class(version_label, "dim-label");
    gtk_box_append(GTK_BOX(vbox), version_label);

    // Separator
    GtkWidget* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(separator, 5);
    gtk_widget_set_margin_bottom(separator, 5);
    gtk_box_append(GTK_BOX(vbox), separator);

    // Description
    const char* description_text =
        "Android UI Hierarchy Viewer\n\n"
        "A tool to view and inspect Android UI layouts using ADB.\n\n"
        "Features:\n"
        "- Connect to Android device via ADB\n"
        "- Dump and display UI hierarchy\n"
        "- Capture and show screenshots\n"
        "- Search and inspect UI nodes\n"
        "- Cache dump files for offline viewing";
    GtkWidget* desc_label = gtk_label_new(description_text);
    gtk_label_set_wrap(GTK_LABEL(desc_label), TRUE);
    gtk_widget_set_halign(desc_label, GTK_ALIGN_START);
    gtk_widget_set_valign(desc_label, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(desc_label), 0.0);
    gtk_box_append(GTK_BOX(vbox), desc_label);

    // Close button
    GtkWidget* close_button = gtk_button_new_with_label("Close");
    gtk_widget_set_halign(close_button, GTK_ALIGN_END);
    gtk_widget_set_margin_top(close_button, 10);
    g_signal_connect_swapped(close_button, "clicked", G_CALLBACK(gtk_window_close), dialog);
    gtk_box_append(GTK_BOX(vbox), close_button);

    gtk_window_present(GTK_WINDOW(dialog));
}

static void main_window_dispose(GObject* object) {
    MainWindow* window = MAIN_WINDOW(object);
    if (window->hierarchy_window) {
        gtk_window_destroy(GTK_WINDOW(window->hierarchy_window));
        window->hierarchy_window = nullptr;
    }
    g_free(window->cached_file);
    window->cached_file = nullptr;
    g_free(window->screenshot_file);
    window->screenshot_file = nullptr;
    G_OBJECT_CLASS(main_window_parent_class)->dispose(object);
}

static void main_window_class_init(MainWindowClass* klass) {
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = main_window_dispose;
}

static void main_window_init(MainWindow* window) {
    window->has_cached_data = FALSE;
    window->cached_file = nullptr;
    window->screenshot_file = nullptr;
    window->hierarchy_window = nullptr;
    window->adb_connected = FALSE;

    gtk_window_set_title(GTK_WINDOW(window), "TopWindow - Android UI Viewer");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 380);

    // Create header bar
    GtkWidget* header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(header_bar), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

    // Create menu button
    GtkWidget* menu_button = gtk_menu_button_new();
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(menu_button), "open-menu-symbolic");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), menu_button);

    // Create menu model
    GMenu* menu = g_menu_new();

    // File submenu
    GMenu* file_submenu = g_menu_new();
    g_menu_append(file_submenu, "Cleanup Cache", "win.cleanup");
    g_menu_append(file_submenu, "Quit", "win.quit");
    g_menu_append_submenu(menu, "File", G_MENU_MODEL(file_submenu));
    g_object_unref(file_submenu);

    // Help submenu
    GMenu* help_submenu = g_menu_new();
    g_menu_append(help_submenu, "About", "win.about");
    g_menu_append_submenu(menu, "Help", G_MENU_MODEL(help_submenu));
    g_object_unref(help_submenu);

    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_button), G_MENU_MODEL(menu));
    g_object_unref(menu);

    // Create actions
    GActionEntry entries[] = {
        { "cleanup", on_cleanup_activate, nullptr, nullptr, nullptr },
        { "quit", on_quit_activate, nullptr, nullptr, nullptr },
        { "about", on_about_activate, nullptr, nullptr, nullptr }
    };
    GSimpleActionGroup* group = g_simple_action_group_new();
    g_action_map_add_action_entries(G_ACTION_MAP(group), entries, G_N_ELEMENTS(entries), window);
    gtk_widget_insert_action_group(GTK_WIDGET(window), "win", G_ACTION_GROUP(group));
    g_object_unref(group);

    window->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(window->vbox, 20);
    gtk_widget_set_margin_end(window->vbox, 20);
    gtk_widget_set_margin_top(window->vbox, 20);
    gtk_widget_set_margin_bottom(window->vbox, 20);
    gtk_window_set_child(GTK_WINDOW(window), window->vbox);

    GtkWidget* title = gtk_label_new("Android UI Hierarchy Viewer");
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_size_new(18 * PANGO_SCALE));
    gtk_label_set_attributes(GTK_LABEL(title), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_append(GTK_BOX(window->vbox), title);

    GtkWidget* info = gtk_label_new("Connect your Android device with USB debugging enabled, then click Startal.");
    gtk_label_set_wrap(GTK_LABEL(info), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(info), 50);
    gtk_widget_set_margin_top(info, 10);
    gtk_box_append(GTK_BOX(window->vbox), info);

    window->adb_status_label = gtk_label_new("ADB Status: Unknown");
    gtk_widget_set_margin_top(window->adb_status_label, 10);
    gtk_box_append(GTK_BOX(window->vbox), window->adb_status_label);

    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_margin_top(button_box, 15);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(window->vbox), button_box);

    window->check_button = gtk_button_new_with_label("Check ADB");
    g_signal_connect(window->check_button, "clicked", G_CALLBACK(on_check_clicked), window);
    gtk_box_append(GTK_BOX(button_box), window->check_button);

    window->startal_button = gtk_button_new_with_label("Startal");
    gtk_box_append(GTK_BOX(button_box), window->startal_button);

    window->refetch_button = gtk_button_new_with_label("🔄 Refetch");
    gtk_widget_set_visible(window->refetch_button, FALSE);
    g_signal_connect(window->refetch_button, "clicked", G_CALLBACK(on_refetch_clicked), window);
    gtk_box_append(GTK_BOX(button_box), window->refetch_button);

    window->status_label = gtk_label_new("Click Check ADB first, then click Startal to begin");
    gtk_widget_set_margin_top(window->status_label, 15);
    gtk_box_append(GTK_BOX(window->vbox), window->status_label);

    window->spinner = gtk_spinner_new();
    gtk_widget_set_margin_top(window->spinner, 10);
    gtk_widget_set_visible(window->spinner, FALSE);
    gtk_box_append(GTK_BOX(window->vbox), window->spinner);

    window->error_label = gtk_label_new("");
    gtk_widget_set_margin_top(window->error_label, 10);
    gtk_label_set_wrap(GTK_LABEL(window->error_label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(window->error_label), 50);
    gtk_widget_set_visible(window->error_label, FALSE);
    gtk_widget_add_css_class(window->error_label, "error");
    PangoAttrList* error_attrs = pango_attr_list_new();
    pango_attr_list_insert(error_attrs, pango_attr_foreground_new(65535, 0, 0));
    gtk_label_set_attributes(GTK_LABEL(window->error_label), error_attrs);
    pango_attr_list_unref(error_attrs);
    gtk_box_append(GTK_BOX(window->vbox), window->error_label);

    g_signal_connect(window->startal_button, "clicked", G_CALLBACK(on_startal_clicked), window);

    AdbManager adb;
    CacheManager cache(adb.get_cache_directory());
    if (cache.has_cached_file()) {
        std::string latest = cache.get_latest_cached_file();
        window->has_cached_data = TRUE;
        window->cached_file = g_strdup(latest.c_str());
        gtk_label_set_text(GTK_LABEL(window->adb_status_label), "✓ Cached Dump Available");
        gtk_widget_add_css_class(window->adb_status_label, "success");
        std::string status = "Cached dump available: " + latest;
        gtk_label_set_text(GTK_LABEL(window->status_label), status.c_str());
    }

    update_button_states(window);
}

MainWindow* main_window_new(GtkApplication* app) {
    return MAIN_WINDOW(g_object_new(MAIN_TYPE_WINDOW, "application", app, nullptr));
}
