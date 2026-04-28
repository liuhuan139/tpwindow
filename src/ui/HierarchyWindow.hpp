#pragma once
#include <gtk/gtk.h>
#include "../models/UiNode.hpp"
#include "NodeAttributesPanel.hpp"

G_BEGIN_DECLS

#define HIERARCHY_TYPE_WINDOW (hierarchy_window_get_type())
#define HIERARCHY_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), HIERARCHY_TYPE_WINDOW, HierarchyWindow))
#define HIERARCHY_IS_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), HIERARCHY_TYPE_WINDOW))

typedef struct _HierarchyWindow HierarchyWindow;
typedef struct _HierarchyWindowClass HierarchyWindowClass;

struct _HierarchyWindow {
    GtkWindow parent;

    GtkWidget* header_bar;
    GtkWidget* search_button;
    GtkWidget* search_box;
    GtkWidget* search_entry;
    GtkWidget* do_search_button;
    GtkWidget* paned;
    GtkWidget* tree_view;
    GtkWidget* scrolled_window;
    NodeAttributesPanel* attributes_panel;
    GtkWidget* thumbnail_button;
    GtkWidget* thumbnail_image;
    GtkWidget* thumbnail_overlay;

    GtkTreeStore* tree_store;
    UiNode* root_node;
    GDestroyNotify root_destroy;
    int rotation;
    gboolean search_visible;
    char* cached_file;
    char* screenshot_file;
};

struct _HierarchyWindowClass {
    GtkWindowClass parent_class;
};

GType hierarchy_window_get_type(void) G_GNUC_CONST;
HierarchyWindow* hierarchy_window_new(GtkWindow* parent, UiNode* root, GDestroyNotify destroy, int rotation, const char* cached_file, const char* screenshot_file);
void hierarchy_window_set_root(HierarchyWindow* window, UiNode* root, GDestroyNotify destroy, int rotation, const char* cached_file);
void hierarchy_window_set_screenshot(HierarchyWindow* window, const char* screenshot_file);

#define THUMBNAIL_TYPE_WINDOW (thumbnail_window_get_type())
#define THUMBNAIL_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), THUMBNAIL_TYPE_WINDOW, ThumbnailWindow))

typedef struct _ThumbnailWindow ThumbnailWindow;
typedef struct _ThumbnailWindowClass ThumbnailWindowClass;

struct _ThumbnailWindow {
    GtkWindow parent;
    GtkWidget* image;
};

struct _ThumbnailWindowClass {
    GtkWindowClass parent_class;
};

GType thumbnail_window_get_type(void) G_GNUC_CONST;
ThumbnailWindow* thumbnail_window_new(GtkWindow* parent, const char* screenshot_file);
void thumbnail_window_set_image(ThumbnailWindow* window, const char* screenshot_file);

G_END_DECLS
