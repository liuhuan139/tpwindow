#include "HierarchyWindow.hpp"
#include "../models/UiTreeModel.hpp"

G_DEFINE_TYPE(HierarchyWindow, hierarchy_window, GTK_TYPE_WINDOW)
G_DEFINE_TYPE(ThumbnailWindow, thumbnail_window, GTK_TYPE_WINDOW)

static void on_selection_changed(GtkTreeSelection* selection, gpointer user_data) {
    HierarchyWindow* window = HIERARCHY_WINDOW(user_data);
    GtkTreeIter iter;
    GtkTreeModel* model;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        UiNode* node = nullptr;
        gtk_tree_model_get(model, &iter, UI_TREE_MODEL_COLUMN_NODE_POINTER, &node, -1);
        node_attributes_panel_set_node(window->attributes_panel, node);
    } else {
        node_attributes_panel_clear(window->attributes_panel);
    }
}

static gboolean search_in_node(UiNode* node, const char* search_text) {
    if (!node || !search_text) return FALSE;

    std::string search_str(search_text);
    std::string class_name = node->get_class();
    std::string text = node->get_text();
    std::string resource_id = node->get_resource_id();

    if (class_name.find(search_str) != std::string::npos ||
        text.find(search_str) != std::string::npos ||
        resource_id.find(search_str) != std::string::npos) {
        return TRUE;
    }

    const auto& attrs = node->get_all_attributes();
    for (const auto& attr : attrs) {
        if (attr.first.find(search_str) != std::string::npos ||
            attr.second.find(search_str) != std::string::npos) {
            return TRUE;
        }
    }

    return FALSE;
}

static gboolean find_and_expand_node(GtkTreeModel* model, GtkTreeIter* parent, const char* search_text, GtkTreeIter* result) {
    GtkTreeIter iter;
    if (!gtk_tree_model_iter_children(model, &iter, parent)) {
        return FALSE;
    }

    do {
        UiNode* node = nullptr;
        gtk_tree_model_get(model, &iter, UI_TREE_MODEL_COLUMN_NODE_POINTER, &node, -1);

        if (search_in_node(node, search_text)) {
            *result = iter;
            GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
            if (path) {
                gtk_tree_view_expand_to_path(GTK_TREE_VIEW(g_object_get_data(G_OBJECT(model), "tree-view")), path);
                gtk_tree_path_free(path);
            }
            return TRUE;
        }

        if (find_and_expand_node(model, &iter, search_text, result)) {
            return TRUE;
        }
    } while (gtk_tree_model_iter_next(model, &iter));

    return FALSE;
}

static void perform_search(HierarchyWindow* window) {
    const char* search_text = gtk_editable_get_text(GTK_EDITABLE(window->search_entry));

    if (!search_text || !*search_text) {
        return;
    }

    GtkTreeModel* model = GTK_TREE_MODEL(window->tree_store);
    g_object_set_data(G_OBJECT(model), "tree-view", window->tree_view);

    GtkTreeIter iter;
    if (find_and_expand_node(model, nullptr, search_text, &iter)) {
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(window->tree_view));
        gtk_tree_selection_select_iter(selection, &iter);
        GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
        if (path) {
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(window->tree_view), path, nullptr, TRUE, 0.5, 0.0);
            gtk_tree_path_free(path);
        }
    }
}

static void on_do_search_clicked(GtkWidget* button, gpointer user_data) {
    HierarchyWindow* window = HIERARCHY_WINDOW(user_data);
    perform_search(window);
}

static void on_search_entry_activate(GtkEntry* entry, gpointer user_data) {
    HierarchyWindow* window = HIERARCHY_WINDOW(user_data);
    perform_search(window);
}

static void on_search_button_clicked(GtkWidget* button, gpointer user_data) {
    HierarchyWindow* window = HIERARCHY_WINDOW(user_data);
    window->search_visible = !window->search_visible;
    gtk_widget_set_visible(window->search_box, window->search_visible);
}

static void on_hierarchy_window_size_allocate(GtkWidget* widget, GdkRectangle* allocation, gpointer user_data) {
    HierarchyWindow* window = HIERARCHY_WINDOW(user_data);
    int max_height = allocation->height / 2;
    gtk_widget_set_size_request(window->thumbnail_overlay, -1, max_height < 120 ? max_height : 120);
}

static void on_thumbnail_clicked(GtkGestureClick* gesture, int n_press, double x, double y, gpointer user_data) {
    HierarchyWindow* window = HIERARCHY_WINDOW(user_data);
    if (window->screenshot_file && g_file_test(window->screenshot_file, G_FILE_TEST_EXISTS)) {
        ThumbnailWindow* thumb_window = thumbnail_window_new(GTK_WINDOW(window), window->screenshot_file);
        gtk_window_present(GTK_WINDOW(thumb_window));
    }
}

static void hierarchy_window_dispose(GObject* object) {
    HierarchyWindow* window = HIERARCHY_WINDOW(object);
    if (window->tree_store) {
        g_object_unref(window->tree_store);
        window->tree_store = nullptr;
    }
    if (window->root_destroy && window->root_node) {
        window->root_destroy(window->root_node);
        window->root_node = nullptr;
    }
    g_free(window->cached_file);
    window->cached_file = nullptr;
    g_free(window->screenshot_file);
    window->screenshot_file = nullptr;
    G_OBJECT_CLASS(hierarchy_window_parent_class)->dispose(object);
}

static void hierarchy_window_class_init(HierarchyWindowClass* klass) {
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = hierarchy_window_dispose;
}

static void hierarchy_window_init(HierarchyWindow* window) {
    window->tree_store = nullptr;
    window->root_node = nullptr;
    window->root_destroy = nullptr;
    window->rotation = 0;
    window->search_visible = FALSE;
    window->cached_file = nullptr;
    window->screenshot_file = nullptr;

    gtk_window_set_title(GTK_WINDOW(window), "UI Hierarchy Viewer");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 900);

    window->header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(window->header_bar), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(window), window->header_bar);

    window->search_button = gtk_button_new_from_icon_name("edit-find-symbolic");
    g_signal_connect(window->search_button, "clicked", G_CALLBACK(on_search_button_clicked), window);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(window->header_bar), window->search_button);

    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(window), main_box);

    window->search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(window->search_box, 10);
    gtk_widget_set_margin_end(window->search_box, 10);
    gtk_widget_set_margin_top(window->search_box, 5);
    gtk_widget_set_margin_bottom(window->search_box, 5);
    gtk_widget_set_visible(window->search_box, FALSE);
    gtk_box_append(GTK_BOX(main_box), window->search_box);

    window->search_entry = gtk_entry_new();
    gtk_widget_set_hexpand(window->search_entry, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(window->search_entry), "Search...");
    gtk_box_append(GTK_BOX(window->search_box), window->search_entry);

    window->do_search_button = gtk_button_new_with_label("Search");
    g_signal_connect(window->do_search_button, "clicked", G_CALLBACK(on_do_search_clicked), window);
    gtk_box_append(GTK_BOX(window->search_box), window->do_search_button);

    g_signal_connect(window->search_entry, "activate", G_CALLBACK(on_search_entry_activate), window);

    window->paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(window->paned), 700);
    gtk_widget_set_vexpand(window->paned, TRUE);
    gtk_box_append(GTK_BOX(main_box), window->paned);

    window->scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(window->scrolled_window, TRUE);
    gtk_widget_set_vexpand(window->scrolled_window, TRUE);
    gtk_paned_set_start_child(GTK_PANED(window->paned), window->scrolled_window);

    window->tree_view = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(window->tree_view), TRUE);
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(window->tree_view), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(window->scrolled_window), window->tree_view);

    GtkTreeViewColumn* col;
    GtkCellRenderer* renderer;

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Class", renderer, "text", UI_TREE_MODEL_COLUMN_CLASS, nullptr);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_min_width(col, 150);
    gtk_tree_view_append_column(GTK_TREE_VIEW(window->tree_view), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Text", renderer, "text", UI_TREE_MODEL_COLUMN_TEXT, nullptr);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_min_width(col, 200);
    gtk_tree_view_append_column(GTK_TREE_VIEW(window->tree_view), col);

    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Resource ID", renderer, "text", UI_TREE_MODEL_COLUMN_RESOURCE_ID, nullptr);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_min_width(col, 200);
    gtk_tree_view_append_column(GTK_TREE_VIEW(window->tree_view), col);

    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(window->tree_view));
    g_signal_connect(selection, "changed", G_CALLBACK(on_selection_changed), window);

    GtkWidget* right_panel_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(right_panel_box, 10);
    gtk_widget_set_margin_end(right_panel_box, 10);
    gtk_widget_set_margin_top(right_panel_box, 10);
    gtk_widget_set_margin_bottom(right_panel_box, 10);
    gtk_paned_set_end_child(GTK_PANED(window->paned), right_panel_box);

    window->thumbnail_overlay = gtk_overlay_new();
    gtk_widget_set_size_request(window->thumbnail_overlay, -1, 120);
    gtk_widget_add_css_class(window->thumbnail_overlay, "card");
    gtk_box_append(GTK_BOX(right_panel_box), window->thumbnail_overlay);

    window->thumbnail_image = gtk_picture_new();
    gtk_widget_set_size_request(window->thumbnail_image, -1, 120);
    gtk_widget_set_hexpand(window->thumbnail_image, TRUE);
    gtk_widget_set_vexpand(window->thumbnail_image, TRUE);
    gtk_picture_set_keep_aspect_ratio(GTK_PICTURE(window->thumbnail_image), TRUE);
    gtk_overlay_set_child(GTK_OVERLAY(window->thumbnail_overlay), window->thumbnail_image);

    GtkWidget* click_overlay = gtk_button_new_with_label("");
    gtk_widget_set_opacity(click_overlay, 0);
    gtk_widget_set_hexpand(click_overlay, TRUE);
    gtk_widget_set_vexpand(click_overlay, TRUE);
    gtk_overlay_add_overlay(GTK_OVERLAY(window->thumbnail_overlay), click_overlay);

    GtkGesture* gesture = gtk_gesture_click_new();
    g_signal_connect(gesture, "pressed", G_CALLBACK(on_thumbnail_clicked), window);
    gtk_widget_add_controller(window->thumbnail_overlay, GTK_EVENT_CONTROLLER(gesture));

    GtkWidget* no_screenshot_label = gtk_label_new("📷\nNo screenshot");
    gtk_label_set_wrap(GTK_LABEL(no_screenshot_label), TRUE);
    gtk_label_set_justify(GTK_LABEL(no_screenshot_label), GTK_JUSTIFY_CENTER);
    gtk_overlay_add_overlay(GTK_OVERLAY(window->thumbnail_overlay), no_screenshot_label);

    window->attributes_panel = node_attributes_panel_new();
    gtk_widget_set_vexpand(GTK_WIDGET(window->attributes_panel), TRUE);
    gtk_box_append(GTK_BOX(right_panel_box), GTK_WIDGET(window->attributes_panel));

    g_signal_connect(window, "size-allocate", G_CALLBACK(on_hierarchy_window_size_allocate), window);
}

void hierarchy_window_set_root(HierarchyWindow* window, UiNode* root, GDestroyNotify destroy, int rotation, const char* cached_file) {
    if (window->tree_store) { g_object_unref(window->tree_store); }
    if (window->root_destroy && window->root_node) {
        window->root_destroy(window->root_node);
    }

    window->root_node = root;
    window->root_destroy = destroy;
    window->rotation = rotation;
    g_free(window->cached_file);
    window->cached_file = cached_file ? g_strdup(cached_file) : nullptr;

    if (root) {
        window->tree_store = ui_tree_store_new(root);
        gtk_tree_view_set_model(GTK_TREE_VIEW(window->tree_view), GTK_TREE_MODEL(window->tree_store));
    } else {
        gtk_tree_view_set_model(GTK_TREE_VIEW(window->tree_view), nullptr);
        window->tree_store = nullptr;
    }

    node_attributes_panel_clear(window->attributes_panel);
}

void hierarchy_window_set_screenshot(HierarchyWindow* window, const char* screenshot_file) {
    g_free(window->screenshot_file);
    window->screenshot_file = screenshot_file ? g_strdup(screenshot_file) : nullptr;

    if (screenshot_file && g_file_test(screenshot_file, G_FILE_TEST_EXISTS)) {
        GFile* file = g_file_new_for_path(screenshot_file);
        gtk_picture_set_file(GTK_PICTURE(window->thumbnail_image), file);
        g_object_unref(file);
    } else {
        gtk_picture_set_paintable(GTK_PICTURE(window->thumbnail_image), nullptr);
    }
}

HierarchyWindow* hierarchy_window_new(GtkWindow* parent, UiNode* root, GDestroyNotify destroy, int rotation, const char* cached_file, const char* screenshot_file) {
    HierarchyWindow* window = HIERARCHY_WINDOW(g_object_new(HIERARCHY_TYPE_WINDOW, "transient-for", parent, nullptr));
    hierarchy_window_set_root(window, root, destroy, rotation, cached_file);
    hierarchy_window_set_screenshot(window, screenshot_file);
    return window;
}

static void thumbnail_window_dispose(GObject* object) {
    G_OBJECT_CLASS(thumbnail_window_parent_class)->dispose(object);
}

static void thumbnail_window_class_init(ThumbnailWindowClass* klass) {
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = thumbnail_window_dispose;
}

static void thumbnail_window_init(ThumbnailWindow* window) {
    gtk_window_set_title(GTK_WINDOW(window), "Screenshot Preview");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget* scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_window_set_child(GTK_WINDOW(window), scrolled);

    window->image = gtk_picture_new();
    gtk_picture_set_keep_aspect_ratio(GTK_PICTURE(window->image), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), window->image);
}

void thumbnail_window_set_image(ThumbnailWindow* window, const char* screenshot_file) {
    if (screenshot_file && g_file_test(screenshot_file, G_FILE_TEST_EXISTS)) {
        GFile* file = g_file_new_for_path(screenshot_file);
        gtk_picture_set_file(GTK_PICTURE(window->image), file);
        g_object_unref(file);
    } else {
        gtk_picture_set_paintable(GTK_PICTURE(window->image), nullptr);
    }
}

ThumbnailWindow* thumbnail_window_new(GtkWindow* parent, const char* screenshot_file) {
    ThumbnailWindow* window = THUMBNAIL_WINDOW(g_object_new(THUMBNAIL_TYPE_WINDOW, "transient-for", parent, nullptr));
    thumbnail_window_set_image(window, screenshot_file);
    return window;
}
