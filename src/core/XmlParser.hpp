#pragma once
#include <memory>
#include <string>
#include "../models/UiNode.hpp"

class XmlParser {
public:
    XmlParser();
    ~XmlParser();

    std::unique_ptr<UiNode> parse_file(const std::string& file_path);
    int get_rotation() const { return rotation_; }

    std::string get_error_message() const { return error_message_; }

private:
    bool parse_with_libxml(const std::string& file_path);

    std::string error_message_;
    std::unique_ptr<UiNode> root_node_;
    int rotation_;
};
