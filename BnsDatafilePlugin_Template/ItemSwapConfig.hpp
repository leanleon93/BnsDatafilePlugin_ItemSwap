#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct ItemSwapConfig {
	struct Swap {
		std::wstring Name = L"";
		int From = 0;
		int To = 0;
		bool Enabled = false;

		bool operator==(const Swap& other) const {
			return Name == other.Name && From == other.From && To == other.To && Enabled == other.Enabled;
		}
	};
	bool GlobalEnabled = false;
	std::vector<Swap> Swaps = {};
	std::unordered_map<int, int> GetEnabledSwapMap() const {
		std::unordered_map<int, int> map;
		if (!GlobalEnabled) return map;
		for (const auto& swap : Swaps) {
			if (swap.Enabled) {
				map[swap.From] = swap.To;
			}
		}
		return map;
	}
};