#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

class UiNode {
public:
    using AttributeMap = std::unordered_map<std::string, std::string>;

    UiNode();
    ~UiNode();

    std::string get_attribute(const std::string& name) const;
    void set_attribute(const std::string& name, const std::string& value);
    bool has_attribute(const std::string& name) const;
    const AttributeMap& get_all_attributes() const;

    void add_child(std::unique_ptr<UiNode> child);
    UiNode* get_parent() const;
    const std::vector<std::unique_ptr<UiNode>>& get_children() const;
    size_t get_child_count() const;
    UiNode* get_child(size_t index) const;

    std::string get_class() const { return get_attribute("class"); }
    std::string get_text() const { return get_attribute("text"); }
    std::string get_resource_id() const { return get_attribute("resource-id"); }
    std::string get_bounds() const { return get_attribute("bounds"); }
    std::string get_index() const { return get_attribute("index"); }
    std::string get_package() const { return get_attribute("package"); }
    bool is_clickable() const;
    bool is_enabled() const;

    std::string get_display_text() const;

private:
    AttributeMap attributes_;
    UiNode* parent_;
    std::vector<std::unique_ptr<UiNode>> children_;
};
