#pragma once
#include <gtk/gtk.h>
#include "../models/UiNode.hpp"

G_BEGIN_DECLS

#define NODE_TYPE_ATTRIBUTES_PANEL (node_attributes_panel_get_type())
#define NODE_ATTRIBUTES_PANEL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), NODE_TYPE_ATTRIBUTES_PANEL, NodeAttributesPanel))
#define NODE_IS_ATTRIBUTES_PANEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), NODE_TYPE_ATTRIBUTES_PANEL))

typedef struct _NodeAttributesPanel NodeAttributesPanel;
typedef struct _NodeAttributesPanelClass NodeAttributesPanelClass;

struct _NodeAttributesPanel {
    GtkBox parent;

    GtkWidget* title_label;
    GtkWidget* grid;
    GtkWidget* scrolled_window;

    UiNode* current_node;
};

struct _NodeAttributesPanelClass {
    GtkBoxClass parent_class;
};

GType node_attributes_panel_get_type(void) G_GNUC_CONST;
NodeAttributesPanel* node_attributes_panel_new();
void node_attributes_panel_set_node(NodeAttributesPanel* panel, UiNode* node);
void node_attributes_panel_clear(NodeAttributesPanel* panel);

G_END_DECLS
