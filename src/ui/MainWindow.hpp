#pragma once
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MAIN_TYPE_WINDOW (main_window_get_type())
#define MAIN_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), MAIN_TYPE_WINDOW, MainWindow))

typedef struct _MainWindow MainWindow;
typedef struct _MainWindowClass MainWindowClass;

struct _MainWindow {
    GtkApplicationWindow parent;

    GtkWidget* startal_button;
    GtkWidget* refetch_button;
    GtkWidget* check_button;
    GtkWidget* status_label;
    GtkWidget* adb_status_label;
    GtkWidget* error_label;
    GtkWidget* spinner;
    GtkWidget* vbox;

    gboolean has_cached_data;
    char* cached_file;
    char* screenshot_file;
    GtkWidget* hierarchy_window;
    gboolean adb_connected;
};

struct _MainWindowClass {
    GtkApplicationWindowClass parent_class;
};

GType main_window_get_type(void) G_GNUC_CONST;
MainWindow* main_window_new(GtkApplication* app);

G_END_DECLS
