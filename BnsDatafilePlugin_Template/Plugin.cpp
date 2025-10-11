#include "DatafilePluginsdk.h"
#include <EU/item/AAA_item_RecordBase.h>
#include <EU/text/AAA_text_RecordBase.h>
#include <EU/BnsTableNames.h>
#include <KR/item/AAA_item_RecordBase.h>
#include <KR/text/AAA_text_RecordBase.h>
#include <KR/BnsTableNames.h>
#include "plugin_version.h"
#include "PluginConfig.hpp"

#include <windows.h>
#include <xorstr.hpp>

/**
 * @file Plugin.cpp
 * @brief Implementation for an ItemSwap datafile plugin using the BnsDatafilePlugin SDK.
 * @author LEaN
 */

 /// @brief Handle for the registered ImGui panel.
static int g_panelHandle = 0;

/// @brief Function pointer to register an ImGui panel.
static RegisterImGuiPanelFn g_register = nullptr;

/// @brief Function pointer to unregister an ImGui panel.
static UnregisterImGuiPanelFn g_unregister = nullptr;

/// @brief Pointer to the ImGui API provided by the host application.
static PluginImGuiAPI* g_imgui = nullptr;
static Data::DataManager* g_dataManager = nullptr;
static DrEl* (__fastcall* g_oFind)(DrMultiKeyTable* thisptr, unsigned __int64 key) = nullptr;

/// @brief Name of the plugin, used for identification and UI.
const char* g_pluginName = "ItemSwap";

static PluginReturnData __fastcall ItemDetour(PluginExecuteParams* params) {
#ifdef _BNSEU
	PLUGIN_DETOUR_GUARD(params, BnsTables::EU::TableNames::GetTableVersion);
#elif _BNSKR
	PLUGIN_DETOUR_GUARD(params, BnsTables::KR::TableNames::GetTableVersion);
#endif
	if (!g_PluginConfig->ItemSwapConfig.GlobalEnabled) return {};
	const auto swapMap = g_PluginConfig->ItemSwapConfig.GetEnabledSwapMap();
	if (swapMap.empty()) return {};
#ifdef _BNSKR
	BnsTables::KR::item_Record::Key currentKey{};
	currentKey.key = params->key;
#elif _BNSEU
	BnsTables::EU::item_Record::Key currentKey{};
	currentKey.key = params->key;
#endif // _BNSKR
	auto it = swapMap.find(currentKey.id);
	if (it != swapMap.end()) {
		currentKey.id = it->second;
		currentKey.level = 1;
		auto recordBase = params->oFind(params->table, currentKey.key);
		if (recordBase != nullptr) {
			return { recordBase };
		}
	}
	return {};
}

// Converts a std::wstring to a std::string (UTF-8)
inline static std::string WStringToString(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
	return strTo;
}

//std::string(UTF-8) to std::wstring
inline static std::wstring StringToWString(const std::string& str) {
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

struct ItemBrowserEntry {
	int id = 0;
	__int16 level = 0;
	std::wstring name = L"";
};

static void FillItemCache(std::unordered_map<unsigned __int64, ItemBrowserEntry>& itemCache) {
	if (g_dataManager == nullptr || g_PluginConfig == nullptr) return;
	auto itemTable = GetTable(g_dataManager, L"item");
#ifdef _BNSEU
	ForEachRecord<BnsTables::EU::item_Record>(itemTable, [&](BnsTables::EU::item_Record* record, size_t) {
#elif _BNSKR
	ForEachRecord<BnsTables::KR::item_Record>(itemTable, [&](BnsTables::KR::item_Record* record, size_t) {
#endif
		if (record == nullptr) return true;
		ItemBrowserEntry entry;
		entry.id = record->key.id;
		entry.level = record->key.level;
		if (record->name2.Key != 0) {
#ifdef _BNSEU
			auto itemName = GetText<BnsTables::EU::text_Record>(g_dataManager, record->name2.Key, g_oFind);
#elif _BNSKR
			auto itemName = GetText<BnsTables::KR::text_Record>(g_dataManager, record->name2.Key, g_oFind);
#endif
			if (itemName && itemName->text.ReadableText) {
				entry.name = itemName->text.ReadableText;
			}
			else {
				entry.name = L"(No Name)";
			}
		}
		itemCache[record->key.key] = entry;
		return true; // continue iteration
		});
}

// Copies a UTF-8 std::string to the Windows clipboard
inline static void CopyStringToClipboard(const std::string & str) {
	if (str.empty()) return;

	// Convert UTF-8 std::string to UTF-16 wstring
	int wlen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
	if (wlen == 0) return;
	std::wstring wstr(wlen, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], wlen);

	// Open clipboard
	if (!OpenClipboard(nullptr)) return;
	EmptyClipboard();

	// Allocate global memory for the clipboard data
	size_t bytes = (wstr.size() + 1) * sizeof(wchar_t);
	HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (!hGlob) {
		CloseClipboard();
		return;
	}

	// Copy the string into the global memory
	void* pGlob = GlobalLock(hGlob);
	if (pGlob) {
		memcpy(pGlob, wstr.c_str(), bytes);
		GlobalUnlock(hGlob);
	}
	// Set clipboard data as Unicode text
	SetClipboardData(CF_UNICODETEXT, hGlob);

	// Close clipboard (do not free hGlob, clipboard owns it now)
	CloseClipboard();
}

#include <sstream>

// Returns true if all words in 'needle' appear in order in 'haystack'
static bool WordsInOrderMatch(const std::string & haystack, const std::string & needle) {
	std::istringstream needleStream(needle);
	std::string needleWord;
	size_t pos = 0;
	while (needleStream >> needleWord) {
		pos = haystack.find(needleWord, pos);
		if (pos == std::string::npos)
			return false;
		pos += needleWord.length();
	}
	return true;
}

static bool itemBrowserOpen = false;
static int newSwapFrom = 0;
static int newSwapTo = 0;

static void ItemBrowserUi(void* userData) {
	if (!itemBrowserOpen) return;
	static std::unordered_map<unsigned __int64, ItemBrowserEntry> itemCache;
	static bool firstOpen = true;
	if (itemCache.empty()) {
		FillItemCache(itemCache);
	}
	if (firstOpen) {
		g_imgui->SetNextWindowSize(600.0f, 400.0f, 1);
		firstOpen = false;
	}
	g_imgui->Begin("Item Browser (Beta)", &itemBrowserOpen, 32);
	static char searchBuffer[128] = "";
	//display all entries from itemCache that match the searchBuffer
	g_imgui->InputText("Search", searchBuffer, sizeof(searchBuffer));
	g_imgui->Separator();
	std::vector<const ItemBrowserEntry*> sortedEntries;
	for (const auto& kv : itemCache) {
		sortedEntries.push_back(&kv.second);
	}
	std::sort(sortedEntries.begin(), sortedEntries.end(), [](const ItemBrowserEntry* a, const ItemBrowserEntry* b) {
		if (a->id != b->id) return a->id < b->id;
		return a->level < b->level;
		});
	std::string searchStr = searchBuffer;
	std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

	for (const auto* entry : sortedEntries) {
		g_imgui->PushIdInt(entry->id * 1000 + entry->level);
		std::string nameStr = WStringToString(entry->name);
		std::string nameStrLower = nameStr;
		std::transform(nameStrLower.begin(), nameStrLower.end(), nameStrLower.begin(), ::tolower);

		bool show = true;
		if (!searchStr.empty()) {
			show = WordsInOrderMatch(nameStrLower, searchStr);
		}
		if (!show) {
			g_imgui->PopId();
			continue;
		}
		g_imgui->Text("ID: %d, Level: %d, Name: %s", entry->id, entry->level, nameStr.c_str());
		g_imgui->SameLine(0, 5.0f);
		if (g_imgui->SmallButton("Copy ID")) {
			CopyStringToClipboard(std::to_string(entry->id));
		}
		g_imgui->SameLine(0, 5.0f);
		if (g_imgui->SmallButton("Set from")) {
			newSwapFrom = entry->id;
		}
		g_imgui->SameLine(0, 5.0f);
		if (g_imgui->SmallButton("Set to")) {
			newSwapTo = entry->id;
		}
		g_imgui->PopId();
	}
	g_imgui->End();
}

/**
 * @brief ImGui panel callback for the plugin.
 *
 * This function is called to render the plugin's configuration panel in the UI.
 *
 * @param userData Optional user data pointer (unused in this template).
 */
static void ItemSwapConfigUi(void* userData) {
	if (g_imgui == nullptr || g_PluginConfig == nullptr) {
		return;
	}
	static char newSwapName[128] = "";
	g_imgui->Checkbox("Enable Item Swap Plugin", &g_PluginConfig->ItemSwapConfig.GlobalEnabled);
	g_imgui->Separator();
	if (g_imgui->Button("Open Item Browser")) {
		itemBrowserOpen = true;
	}
	g_imgui->Separator();
	if (g_imgui->CollapsingHeader("Item Swaps")) {
		for (auto& swap : g_PluginConfig->ItemSwapConfig.Swaps) {
			g_imgui->PushIdInt(&swap - &g_PluginConfig->ItemSwapConfig.Swaps[0]);
			g_imgui->Checkbox(WStringToString(swap.Name).c_str(), &swap.Enabled);
			g_imgui->SameLine(0, 5.0f);
			g_imgui->Text("From ID: %d", swap.From);
			g_imgui->SameLine(0, 5.0f);
			g_imgui->Text("To ID: %d", swap.To);
			g_imgui->SameLine(0, 5.0f);
			//edit. fill the newswap fields
			if (g_imgui->SmallButton("Edit")) {
				strncpy_s(newSwapName, WStringToString(swap.Name).c_str(), sizeof(newSwapName) - 1);
				newSwapFrom = swap.From;
				newSwapTo = swap.To;
				g_PluginConfig->ItemSwapConfig.Swaps.erase(
					std::remove(g_PluginConfig->ItemSwapConfig.Swaps.begin(), g_PluginConfig->ItemSwapConfig.Swaps.end(), swap),
					g_PluginConfig->ItemSwapConfig.Swaps.end()
				);
				g_imgui->PopId();
			}
			g_imgui->SameLine(0, 5.0f);
			if (g_imgui->SmallButton("Remove")) {
				//remove this swap
				g_PluginConfig->ItemSwapConfig.Swaps.erase(
					std::remove(g_PluginConfig->ItemSwapConfig.Swaps.begin(), g_PluginConfig->ItemSwapConfig.Swaps.end(), swap),
					g_PluginConfig->ItemSwapConfig.Swaps.end()
				);
				g_imgui->PopId();
				break; //break out of the loop since the vector has changed
			}
			g_imgui->PopId();
		}
	}
	g_imgui->Spacing();
	g_imgui->Separator();
	g_imgui->Spacing();
	//add a new swap 
	g_imgui->InputText("New Swap Name", newSwapName, sizeof(newSwapName));
	g_imgui->InputInt("From ID", &newSwapFrom);
	g_imgui->InputInt("To ID", &newSwapTo);
	if (g_imgui->Button("Add")) {
		if (strlen(newSwapName) > 0 && newSwapFrom != 0 && newSwapTo != 0) {
			ItemSwapConfig::Swap swap;
			swap.Name = StringToWString(std::string(newSwapName));
			swap.From = newSwapFrom;
			swap.To = newSwapTo;
			swap.Enabled = true;
			g_PluginConfig->ItemSwapConfig.Swaps.push_back(swap);
			//clear input
			newSwapName[0] = '\0';
			newSwapFrom = 0;
			newSwapTo = 0;
		}
	}
	g_imgui->Spacing();
	g_imgui->Separator();
	g_imgui->Spacing();
	if (g_imgui->Button("Save Configuration")) {
		g_PluginConfig->SaveToDisk();
	}
	if (g_imgui->Button("Reload Configuration")) {
		g_PluginConfig->ReloadFromDisk();
	}
	ItemBrowserUi(nullptr);
}

/**
 * @brief Initializes the plugin and registers the ImGui panel.
 *
 * Called by the host application when the plugin is loaded.
 *
 * @param params Initialization parameters provided by the host.
 */
static void __fastcall Init(PluginInitParams * params) {
	if (params && params->registerImGuiPanel && params->unregisterImGuiPanel && params->imgui)
	{
		g_imgui = params->imgui;
		g_register = params->registerImGuiPanel;
		g_unregister = params->unregisterImGuiPanel;
		ImGuiPanelDesc desc = { g_pluginName, ItemSwapConfigUi, nullptr };
		g_panelHandle = g_register(&desc, false);
	}
	if (params && params->dataManager && params->oFind) {
		g_dataManager = params->dataManager;
		g_oFind = params->oFind;
	}
	g_PluginConfig = std::make_unique<PluginConfig>();
}


/**
 * @brief Unregisters the ImGui panel and performs cleanup.
 *
 * Called by the host application when the plugin is unloaded.
 */
static void __fastcall Unregister() {
	if (g_unregister && g_panelHandle != 0) {
		g_unregister(g_panelHandle);
		g_panelHandle = 0;
	}
}

/// @brief Table handler array mapping table names to detour functions.
PluginTableHandler handlers[] = {
	{ L"item", &ItemDetour }
};

// Plugin metadata and registration macros
DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER(g_pluginName)
DEFINE_PLUGIN_VERSION(PLUGIN_VERSION)
DEFINE_PLUGIN_INIT(Init, Unregister)
DEFINE_PLUGIN_TABLE_HANDLERS(handlers)