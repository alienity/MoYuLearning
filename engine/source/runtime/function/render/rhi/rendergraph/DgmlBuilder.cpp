#include "DgmlBuilder.h"
#include "XmlWriter.h"
#include <fstream>

DgmlBuilder::DgmlBuilder(const std::string& Title, Dgml::GraphDirection Direction /*= Dgml::GraphDirection::Default*/) :
    Graph(Title, Direction)
{}

Dgml::Node* DgmlBuilder::AddNode(std::string_view Id, std::string_view Label, std::string_view Category, std::string_view Background)
{
	auto Node = Graph.AddNode();
	Node->Id = Id;
	Node->Label = Label;
	Node->Category = Category;
	Node->Background = Background;
    return Node;
}

Dgml::Link* DgmlBuilder::AddLink(const std::string& Source, const std::string& Target, std::string_view Label, std::string_view Category)
{
	auto Link = Graph.AddLink();
	Link->Source = Source;
	Link->Target = Target;
	Link->Label = Label;
	Link->Category = Category;
	return Link;
}

Dgml::Category* DgmlBuilder::AddCategory(const std::string& Id, const std::string& Label, std::string_view Background)
{
	auto Category = Graph.AddCategory();
	Category->Id = Id;
	Category->Label = Label;
	Category->Background = Background;
	return Category;
}

void DgmlBuilder::SaveAs(const std::filesystem::path& Path) const
{
    std::ofstream FileStream;
    FileStream.open(Path);
    XmlWriter::Xml(FileStream);
    Graph.Serialize(FileStream);
    FileStream.close();
}
