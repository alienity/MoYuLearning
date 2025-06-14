#pragma once
#include "Dgml.h"
#include <filesystem>

class DgmlBuilder
{
public:
    explicit DgmlBuilder(const std::string& Title, Dgml::GraphDirection Direction = Dgml::GraphDirection::Default);

    Dgml::Node* AddNode(std::string_view Id, std::string_view Label, std::string_view Category, std::string_view Background);

    Dgml::Link* AddLink(const std::string& Source, const std::string& Target, std::string_view Label, std::string_view Category);

    Dgml::Category* AddCategory(const std::string& Id, const std::string& Label, std::string_view Background);

    void SaveAs(const std::filesystem::path& Path) const;

private:
    Dgml::Graph Graph;
};
