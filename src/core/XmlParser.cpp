#include "XmlParser.hpp"
#include <libxml/parser.h>
#include <libxml/tree.h>

XmlParser::XmlParser()
    : rotation_(0) {
}

XmlParser::~XmlParser() {
}

static void parse_node_attributes(xmlNode* xml_node, UiNode* ui_node) {
    for (xmlAttr* attr = xml_node->properties; attr; attr = attr->next) {
        xmlChar* value = xmlNodeListGetString(xml_node->doc, attr->children, 1);
        if (value) {
            ui_node->set_attribute(reinterpret_cast<const char*>(attr->name),
                                  reinterpret_cast<const char*>(value));
            xmlFree(value);
        }
    }
}

static UiNode* build_tree_recursive(xmlNode* xml_node, UiNode* parent) {
    if (!xml_node) return nullptr;

    for (xmlNode* node = xml_node; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE) {
            if (xmlStrcmp(node->name, BAD_CAST "node") == 0) {
                auto ui_node = std::make_unique<UiNode>();
                parse_node_attributes(node, ui_node.get());
                UiNode* ptr = ui_node.get();
                if (parent) {
                    parent->add_child(std::move(ui_node));
                }
                build_tree_recursive(node->children, ptr);
                if (!parent) {
                    return ptr;
                }
            } else {
                build_tree_recursive(node->children, parent);
            }
        }
    }
    return nullptr;
}

bool XmlParser::parse_with_libxml(const std::string& file_path) {
    xmlDoc* doc = xmlReadFile(file_path.c_str(), nullptr, 0);
    if (!doc) {
        error_message_ = "Failed to parse XML file: " + file_path;
        return false;
    }

    xmlNode* root_element = xmlDocGetRootElement(doc);
    if (!root_element) {
        xmlFreeDoc(doc);
        error_message_ = "Empty XML document";
        return false;
    }

    if (xmlStrcmp(root_element->name, BAD_CAST "hierarchy") != 0) {
        xmlFreeDoc(doc);
        error_message_ = "Unexpected root element, expected 'hierarchy'";
        return false;
    }

    for (xmlAttr* attr = root_element->properties; attr; attr = attr->next) {
        if (xmlStrcmp(attr->name, BAD_CAST "rotation") == 0) {
            xmlChar* value = xmlNodeListGetString(root_element->doc, attr->children, 1);
            if (value) {
                rotation_ = std::stoi(reinterpret_cast<const char*>(value));
                xmlFree(value);
            }
        }
    }

    root_node_ = std::make_unique<UiNode>();
    root_node_->set_attribute("class", "hierarchy");
    build_tree_recursive(root_element->children, root_node_.get());

    xmlFreeDoc(doc);
    return true;
}

std::unique_ptr<UiNode> XmlParser::parse_file(const std::string& file_path) {
    error_message_.clear();
    rotation_ = 0;
    root_node_.reset();

    if (parse_with_libxml(file_path)) {
        return std::move(root_node_);
    }
    return nullptr;
}
