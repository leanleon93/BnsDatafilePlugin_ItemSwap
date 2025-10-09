#include "DatafilePluginsdk.h"
#include <EU/item/AAA_item_RecordBase.h>
#include <EU/BnsTableNames.h>
#include "plugin_version.h"

/**
 * @file Plugin.cpp
 * @brief Template implementation for a datafile plugin using the BnsDatafilePlugin SDK.
 * @author LEaN
 *
 * This file provides a starting point for plugin creators, including example hooks,
 * ImGui panel registration, and plugin metadata definitions.
 */

 /// @brief Handle for the registered ImGui panel.
static int g_panelHandle = 0;

/// @brief Function pointer to register an ImGui panel.
static RegisterImGuiPanelFn g_register = nullptr;

/// @brief Function pointer to unregister an ImGui panel.
static UnregisterImGuiPanelFn g_unregister = nullptr;

/// @brief Pointer to the ImGui API provided by the host application.
static PluginImGuiAPI* g_imgui = nullptr;

/// @brief Name of the plugin, used for identification and UI.
const char* g_pluginName = "DatafilePluginTemplate";

/**
 * @brief Example detour function for the "item" table.
 *
 * This function demonstrates how to intercept lookups for items in the datafile.
 * Plugin authors can modify or redirect lookups as needed.
 *
 * @param params Parameters for the plugin execution, including the table and key.
 * @return PluginReturnData Optionally returns a replacement element. Only return if you absolutely have to. Prefer modifying the element in place.
 */
static PluginReturnData __fastcall DatafileItemDetour(PluginExecuteParams* params) {
	PLUGIN_DETOUR_GUARD(params, BnsTables::EU::TableNames::GetTableVersion);
	unsigned __int64 key = params->key;

	// Example to replace one item with another when the game looks it up

	//if (key == 4295877296) {
	//	BnsTables::Shared::DrEl* result = params->oFind(params->table, 4295917336);
	//	//params->displaySystemChatMessage(L"ExampleItemPlugin: Redirected item key 4295902840 to 4294967396", false);
	//	return { result };
	//}

	return {};
}

/**
 * @brief ImGui panel callback for the plugin.
 *
 * This function is called to render the plugin's configuration panel in the UI.
 *
 * @param userData Optional user data pointer (unused in this template).
 */
static void TemplateUiPanel(void* userData) {
	g_imgui->Text("This is a template plugin.");
	g_imgui->Text("You can use this panel to configure your plugin.");
	g_imgui->Separator();
	g_imgui->Text("This plugin does not do anything yet.");
}

/**
 * @brief Initializes the plugin and registers the ImGui panel.
 *
 * Called by the host application when the plugin is loaded.
 *
 * @param params Initialization parameters provided by the host.
 */
static void __fastcall Init(PluginInitParams* params) {
	if (params && params->registerImGuiPanel && params->unregisterImGuiPanel && params->imgui)
	{
		g_imgui = params->imgui;
		g_register = params->registerImGuiPanel;
		g_unregister = params->unregisterImGuiPanel;
		ImGuiPanelDesc desc = { g_pluginName, TemplateUiPanel, nullptr };
		g_panelHandle = g_register(&desc, false);
	}
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
	{ L"item", &DatafileItemDetour }
};

// Plugin metadata and registration macros
DEFINE_PLUGIN_API_VERSION()
DEFINE_PLUGIN_IDENTIFIER(g_pluginName)
DEFINE_PLUGIN_VERSION(PLUGIN_VERSION)
DEFINE_PLUGIN_INIT(Init, Unregister)
DEFINE_PLUGIN_TABLE_HANDLERS(handlers)