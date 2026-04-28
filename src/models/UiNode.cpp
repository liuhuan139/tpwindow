#include "UiNode.hpp"
#include <sstream>
#include <algorithm>

UiNode::UiNode()
    : parent_(nullptr) {
}

UiNode::~UiNode() = default;

std::string UiNode::get_attribute(const std::string& name) const {
    auto it = attributes_.find(name);
    if (it != attributes_.end()) {
        return it->second;
    }
    return "";
}

void UiNode::set_attribute(const std::string& name, const std::string& value) {
    attributes_[name] = value;
}

bool UiNode::has_attribute(const std::string& name) const {
    return attributes_.find(name) != attributes_.end();
}

const UiNode::AttributeMap& UiNode::get_all_attributes() const {
    return attributes_;
}

void UiNode::add_child(std::unique_ptr<UiNode> child) {
    child->parent_ = this;
    children_.push_back(std::move(child));
}

UiNode* UiNode::get_parent() const {
    return parent_;
}

const std::vector<std::unique_ptr<UiNode>>& UiNode::get_children() const {
    return children_;
}

size_t UiNode::get_child_count() const {
    return children_.size();
}

UiNode* UiNode::get_child(size_t index) const {
    if (index < children_.size()) {
        return children_[index].get();
    }
    return nullptr;
}

bool UiNode::is_clickable() const {
    std::string val = get_attribute("clickable");
    return val == "true";
}

bool UiNode::is_enabled() const {
    std::string val = get_attribute("enabled");
    return val != "false";
}

std::string UiNode::get_display_text() const {
    std::ostringstream oss;
    std::string cls = get_class();
    size_t last_dot = cls.rfind('.');
    if (last_dot != std::string::npos) {
        oss << cls.substr(last_dot + 1);
    } else {
        oss << cls;
    }
    std::string txt = get_text();
    if (!txt.empty()) {
        oss << " \"" << txt.substr(0, 30);
        if (txt.length() > 30) oss << "...";
        oss << "\"";
    }
    std::string res_id = get_resource_id();
    if (!res_id.empty()) {
        size_t last_slash = res_id.rfind('/');
        if (last_slash != std::string::npos) {
            oss << " [" << res_id.substr(last_slash + 1) << "]";
        } else {
            oss << " [" << res_id << "]";
        }
    }
    return oss.str();
}
