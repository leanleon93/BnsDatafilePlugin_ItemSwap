#include "xorstr.hpp"
#include "PluginConfig.hpp"
#include "pugixml/pugixml.hpp"
#include <windows.h>

std::unique_ptr<PluginConfig> g_PluginConfig;

PluginConfig::PluginConfig()
{
	ConfigPath = xorstr_(L"./datafilePlugins/itemswap.xml");
	ReloadFromDisk();
}

static std::wstring PugiCharPtrToWString(const pugi::char_t* charPtr) {
	if (charPtr == nullptr) {
		return L"";
	}
	// Convert pugi::char_t* to char* first
	const char* utf8Str = charPtr;
	// Convert char* to std::wstring
	return std::wstring(utf8Str, utf8Str + strlen(utf8Str));
}

static void SetSwaps(pugi::xml_document const& doc, ItemSwapConfig& itemSwapConfig) {
	for (pugi::xml_node swapNode : doc.child("config").child("swaps").children("swap")) {
		ItemSwapConfig::Swap swap;
		if (pugi::xml_attribute nameAttribute = swapNode.attribute("name"); nameAttribute) {
			swap.Name = PugiCharPtrToWString(nameAttribute.as_string());
		}
		if (pugi::xml_attribute fromAttribute = swapNode.attribute("from"); fromAttribute) {
			swap.From = fromAttribute.as_int();
		}
		if (pugi::xml_attribute toAttribute = swapNode.attribute("to"); toAttribute) {
			swap.To = toAttribute.as_int();
		}
		if (pugi::xml_attribute enabledAttribute = swapNode.attribute("enabled"); enabledAttribute) {
			swap.Enabled = enabledAttribute.as_bool();
		}
		itemSwapConfig.Swaps.push_back(swap);
	}
}

void PluginConfig::ReloadFromDisk()
{
	ItemSwapConfig = { };
	pugi::xml_document doc;
	if (pugi::xml_parse_result result = doc.load_file(ConfigPath.string().c_str()); !result) {
		ItemSwapConfig = {};
		return;
	}
	if (pugi::xml_node enabledNode = doc.child("config").child("global-enabled"); enabledNode) {
		ItemSwapConfig.GlobalEnabled = enabledNode.attribute("value").as_bool();
	}
	SetSwaps(doc, ItemSwapConfig);
}

inline static std::string WStringToString(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
	return strTo;
}

void PluginConfig::SaveToDisk()
{
	pugi::xml_document doc;
	pugi::xml_node configNode = doc.append_child("config");
	configNode.append_child("global-enabled").append_attribute("value") = ItemSwapConfig.GlobalEnabled;

	pugi::xml_node swapsNode = configNode.append_child("swaps");
	for (const auto& swap : ItemSwapConfig.Swaps) {
		pugi::xml_node swapNode = swapsNode.append_child("swap");
		swapNode.append_attribute("name") = WStringToString(swap.Name).c_str();
		swapNode.append_attribute("from") = swap.From;
		swapNode.append_attribute("to") = swap.To;
		swapNode.append_attribute("enabled") = swap.Enabled;
	}
	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";
	decl.append_attribute("encoding") = "UTF-8";
	doc.save_file(ConfigPath.string().c_str(), PUGIXML_TEXT("  "));
}