#pragma once
#include <gtk/gtk.h>
#include "UiNode.hpp"

G_BEGIN_DECLS

enum {
    UI_TREE_MODEL_COLUMN_CLASS,
    UI_TREE_MODEL_COLUMN_TEXT,
    UI_TREE_MODEL_COLUMN_RESOURCE_ID,
    UI_TREE_MODEL_COLUMN_NODE_POINTER,
    UI_TREE_MODEL_N_COLUMNS
};

GtkTreeStore* ui_tree_store_new(UiNode* root);

G_END_DECLS
