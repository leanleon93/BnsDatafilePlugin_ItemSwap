# BNS Datafile Plugin Template [![MSBuild](https://github.com/leanleon93/BnsDatafilePlugin_Template/actions/workflows/msbuild.yml/badge.svg)](https://github.com/leanleon93/BnsDatafilePlugin_Template/actions/workflows/msbuild.yml)

A comprehensive template for creating datafile plugins for Blade & Soul using my BnsDatafilePlugin SDK. This template provides a starting point for developers who want to create plugins that can intercept, modify, or extend game data table lookups.

## 🚀 Features

- **Table Interception**: Hook into game data table lookups and modify behavior
- **ImGui Integration**: Built-in UI panels for plugin configuration and debugging
- **Example Implementation**: Complete working example with item table detour
- **Comprehensive Documentation**: Well-documented code with examples

## 📋 Prerequisites

- **Visual Studio 2022** (v143 toolset)
- **Windows 10 SDK** (10.0.19041.0 or later)
- **DatafilePluginSdk** (included in `deps/DatafilePluginSdk/`)
- **BnsPluginTables** (included in `deps/BnsPluginTables/`)

## 🛠️ Building the Plugin

### Quick Start

1. **Create a new repo from this template and clone it**:
   ```bash
   git clone <repository-url>
   cd BnsDatafilePlugin_Template
   ```

2. **Open the solution**:
   ```bash
   BnsDatafilePlugin_Template.sln
   ```

3. **Build the project**:
   - Select `Debug|x64` or `Release|x64` configuration
   - Build → Build Solution (Ctrl+Shift+B)

4. **Output**:
   - The compiled DLL will be available in `bin/BnsDatafilePlugin_Template/DebugX64/` or `bin/BnsDatafilePlugin_Template/ReleaseX64/`

### Build Configurations

| Configuration | Platform | Output | Use Case |
|--------------|----------|---------|----------|
| Debug | x64 | DatafilePluginTemplate.dll | Development & Testing |
| Release | x64 | DatafilePluginTemplate.dll | Production |

## 🔧 Usage & Customization

### Basic Plugin Structure

The template demonstrates the essential components of a datafile plugin:

```cpp
// 1. Define detour functions for specific tables
static PluginReturnData __fastcall DatafileItemDetour(PluginExecuteParams* params) {
    // Intercept item lookups and optionally modify behavior
    return {};
}

// 2. Create ImGui panels for configuration
static void TemplateUiPanel(void* userData) {
    g_imgui->Text("Plugin configuration goes here");
}

// 3. Register table handlers
PluginTableHandler handlers[] = {
    { L"item", &DatafileItemDetour }
};
```

### Customizing for Your Plugin

1. **Modify Plugin Metadata**:
   ```cpp
   const char* g_pluginName = "YourPluginName";
   DEFINE_PLUGIN_VERSION("1.0.0")
   ```

2. **Add Table Handlers**:
   ```cpp
   PluginTableHandler handlers[] = {
       { L"item", &YourItemDetour },
       { L"skill", &YourSkillDetour },
       // Add more table handlers as needed
   };
   ```

3. **Implement Custom Logic**:
   - Modify the detour functions to implement your specific functionality
   - Use `params->oFind()` to perform original lookups
   - Return `PluginReturnData` with a replacement element if needed
   - Use `params->displaySystemChatMessage()` for ingame chat messages

4. **Customize UI Panel**:
   - Modify `TemplateUiPanel()` to add configuration options
   - Use ImGui API through `g_imgui` pointer

### Available Tables

The plugin can intercept lookups for all game tables (see `deps/BnsPluginTables/Generated/include/EU/`)

## 🎯 Example: Item Redirection

The template includes a commented example showing how to redirect one item to another:

```cpp
static PluginReturnData __fastcall DatafileItemDetour(PluginExecuteParams* params) {
    PLUGIN_DETOUR_GUARD(params, BnsTables::EU::TableNames::GetTableVersion);
    unsigned __int64 key = params->key;
    
    // Redirect item key 4295877296 to 4295917336
    if (key == 4295877296) {
        BnsTables::Shared::DrEl* result = params->oFind(params->table, 4295917336);
        return { result };
    }
    
    return {}; // No modification
}
```

## 🔍 Debugging

### Debug Features

- **System Chat Messages**: Use `params->displaySystemChatMessage()` to output debug info ingame
- **ImGui Panels**: Create interactive debug panels
- **Visual Studio Debugging**: Attach to the game process for step-through debugging

### Guard Macros

```cpp
PLUGIN_DETOUR_GUARD(params, BnsTables::EU::TableNames::GetTableVersion);
```

Use this guard macro at the beginning of detour functions for proper version check and error handling.

## 📚 API Reference

### Core Functions

- `PluginExecuteParams`: Parameters passed to detour functions
- `PluginInitParams`: Initialization parameters
- `PluginReturnData`: Return structure for detour functions
- `PluginTableHandler`: Maps table names to detour functions

### ImGui Integration

Access ImGui functionality through the `g_imgui` pointer:

For example:
```cpp
g_imgui->Text("Hello World");
g_imgui->Button("Click Me");
g_imgui->InputText("Input", buffer, sizeof(buffer));
```

## 🚨 Important Notes

- **Plugin API Version**: Make sure to keep your submodules updated for best compatibility
- **Memory Management**: Be careful with memory allocation/deallocation
- **Game Compatibility**: Test thoroughly with target game version
- **Performance**: Minimize processing in detour functions to avoid game lag

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📄 License

This template is provided as-is for educational and development purposes.

## 🔗 Resources

- [DatafilePluginloader](https://github.com/leanleon93/BnsPlugin_DatafilePluginloader)
- [ImGui Documentation](https://github.com/ocornut/imgui)

---

**Author**: LEaN
