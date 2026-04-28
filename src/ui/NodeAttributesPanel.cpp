#include "NodeAttributesPanel.hpp"
#include <algorithm>

G_DEFINE_TYPE(NodeAttributesPanel, node_attributes_panel, GTK_TYPE_BOX)

static void node_attributes_panel_class_init(NodeAttributesPanelClass* klass) {
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
}

static void node_attributes_panel_init(NodeAttributesPanel* panel) {
    panel->current_node = nullptr;

    gtk_orientable_set_orientation(GTK_ORIENTABLE(panel), GTK_ORIENTATION_VERTICAL);
    gtk_widget_set_size_request(GTK_WIDGET(panel), 300, -1);

    panel->title_label = gtk_label_new("Node Attributes");
    gtk_widget_set_margin_start(panel->title_label, 10);
    gtk_widget_set_margin_end(panel->title_label, 10);
    gtk_widget_set_margin_top(panel->title_label, 10);
    gtk_widget_set_margin_bottom(panel->title_label, 10);
    gtk_widget_set_halign(panel->title_label, GTK_ALIGN_START);
    PangoAttrList* attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_size_new(14 * PANGO_SCALE));
    gtk_label_set_attributes(GTK_LABEL(panel->title_label), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_append(GTK_BOX(panel), panel->title_label);

    panel->scrolled_window = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(panel->scrolled_window, TRUE);
    gtk_widget_set_hexpand(panel->scrolled_window, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(panel->scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    panel->grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(panel->grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(panel->grid), 10);
    gtk_widget_set_margin_start(panel->grid, 10);
    gtk_widget_set_margin_end(panel->grid, 10);
    gtk_widget_set_margin_bottom(panel->grid, 10);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(panel->scrolled_window), panel->grid);
    gtk_box_append(GTK_BOX(panel), panel->scrolled_window);
}

static gboolean is_important_attribute(const std::string& name) {
    return name == "clickable" || name == "enabled" || name == "bounds" ||
           name == "resource-id" || name == "class" || name == "text";
}

NodeAttributesPanel* node_attributes_panel_new() {
    return NODE_ATTRIBUTES_PANEL(g_object_new(NODE_TYPE_ATTRIBUTES_PANEL, NULL));
}

void node_attributes_panel_set_node(NodeAttributesPanel* panel, UiNode* node) {
    node_attributes_panel_clear(panel);
    panel->current_node = node;

    if (!node) return;

    const auto& attrs = node->get_all_attributes();
    std::vector<std::pair<std::string, std::string>> sorted_attrs(attrs.begin(), attrs.end());
    std::sort(sorted_attrs.begin(), sorted_attrs.end(),
              [](const auto& a, const auto& b) {
                  bool a_imp = is_important_attribute(a.first);
                  bool b_imp = is_important_attribute(b.first);
                  if (a_imp != b_imp) return a_imp;
                  return a.first < b.first;
              });

    int row = 0;
    for (const auto& attr : sorted_attrs) {
        GtkWidget* key_label = gtk_label_new((attr.first + ":").c_str());
        gtk_widget_set_halign(key_label, GTK_ALIGN_START);
        gtk_widget_set_valign(key_label, GTK_ALIGN_START);
        PangoAttrList* key_attrs = pango_attr_list_new();
        if (is_important_attribute(attr.first)) {
            pango_attr_list_insert(key_attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        }
        gtk_label_set_attributes(GTK_LABEL(key_label), key_attrs);
        pango_attr_list_unref(key_attrs);

        GtkWidget* value_label = gtk_label_new(attr.second.c_str());
        gtk_widget_set_halign(value_label, GTK_ALIGN_START);
        gtk_widget_set_valign(value_label, GTK_ALIGN_START);
        gtk_label_set_wrap(GTK_LABEL(value_label), TRUE);
        gtk_label_set_wrap_mode(GTK_LABEL(value_label), PANGO_WRAP_WORD_CHAR);
        gtk_widget_set_hexpand(value_label, TRUE);

        gtk_grid_attach(GTK_GRID(panel->grid), key_label, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(panel->grid), value_label, 1, row, 1, 1);
        row++;
    }
}

void node_attributes_panel_clear(NodeAttributesPanel* panel) {
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(panel->grid))) {
        gtk_widget_unparent(child);
    }
    panel->current_node = nullptr;
}
