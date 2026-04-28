#include "UiTreeModel.hpp"

static void add_node_to_store(GtkTreeStore* store, GtkTreeIter* parent, UiNode* node) {
    for (size_t i = 0; i < node->get_child_count(); i++) {
        UiNode* child = node->get_child(i);
        GtkTreeIter iter;

        std::string class_name = child->get_class();
        size_t last_dot = class_name.rfind('.');
        if (last_dot != std::string::npos) {
            class_name = class_name.substr(last_dot + 1);
        }

        gtk_tree_store_append(store, &iter, parent);
        gtk_tree_store_set(store, &iter,
            UI_TREE_MODEL_COLUMN_CLASS, class_name.c_str(),
            UI_TREE_MODEL_COLUMN_TEXT, child->get_text().c_str(),
            UI_TREE_MODEL_COLUMN_RESOURCE_ID, child->get_resource_id().c_str(),
            UI_TREE_MODEL_COLUMN_NODE_POINTER, child,
            -1);

        add_node_to_store(store, &iter, child);
    }
}

GtkTreeStore* ui_tree_store_new(UiNode* root) {
    GtkTreeStore* store = gtk_tree_store_new(UI_TREE_MODEL_N_COLUMNS,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

    if (root) {
        add_node_to_store(store, NULL, root);
    }

    return store;
}
