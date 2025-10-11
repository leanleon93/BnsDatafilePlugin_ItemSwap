#pragma once
#include "ItemSwapConfig.hpp"
#include <filesystem>
#include <unordered_map>

class PluginConfig {
public:
	PluginConfig();
	void ReloadFromDisk();
	void SaveToDisk();
	ItemSwapConfig ItemSwapConfig;
private:
	std::filesystem::path ConfigPath;
};


extern std::unique_ptr<PluginConfig> g_PluginConfig;