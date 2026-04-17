// SSSOAutoConfig — SKSE plugin for Skyrim Save System Overhaul 3
// Author: Young Jonii

#include "PCH.h"

namespace {

    // Convert character name bytes to uppercase hex (matches Skyrim save filename encoding).
    // "Jal" -> "4A616C"
    // Operates on raw bytes, so it transparently handles UTF-8 multi-byte
    // sequences (e.g. Korean Hangul "한" -> "ED959C") as long as Skyrim is
    // using the same encoding when it writes the save filename.
    std::string NameToHex(std::string_view name) {
        std::ostringstream oss;
        for (unsigned char c : name) {
            oss << std::uppercase << std::hex
                << std::setw(2) << std::setfill('0')
                << static_cast<int>(c);
        }
        return oss.str();
    }

    // Convert Skyrim's narrow player name to UTF-16 for the Windows filesystem.
    // Modern Skyrim SE/AE returns UTF-8 internally; older builds and some
    // localized versions (e.g. Korean) may use the system codepage.  Try
    // strict UTF-8 first and fall back to CP_ACP so non-Latin names always
    // produce a valid wide-character path.
    std::wstring NarrowToWide(std::string_view s) {
        if (s.empty()) return {};
        int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                      s.data(), static_cast<int>(s.size()),
                                      nullptr, 0);
        UINT cp = CP_UTF8;
        if (len <= 0) {
            cp = CP_ACP;
            len = MultiByteToWideChar(CP_ACP, 0,
                                      s.data(), static_cast<int>(s.size()),
                                      nullptr, 0);
            if (len <= 0) return {};
        }
        std::wstring out(static_cast<std::size_t>(len), L'\0');
        MultiByteToWideChar(cp, 0, s.data(), static_cast<int>(s.size()),
                            out.data(), len);
        return out;
    }

    // Get the saves directory.  SSE and VR use different "My Games" sub-folders.
    fs::path GetSavesDirectory() {
        PWSTR docs = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)))
            return {};

        const char* gameDir = REL::Module::IsVR() ? "Skyrim VR" : "Skyrim Special Edition";
        fs::path p = fs::path(docs) / "My Games" / gameDir / "Saves";
        CoTaskMemFree(docs);
        return p;
    }

    // Save filenames:
    //   Save1_8981A6BA_0_4A616C_Tamriel_000113_20260406140126_1_1
    // Character ID = segment after first '_' through the hex-encoded name.
    // The hex name must be followed by '_' or end-of-string to avoid false
    // matches inside the hash (e.g. name "A" = hex "41" matching in "B41FD7C2").
    std::string ExtractCharacterID(std::string_view filename, std::string_view hexName) {
        // Search for hexName at a valid word boundary
        std::size_t searchFrom = 0;
        while (searchFrom < filename.size()) {
            auto hexPos = filename.find(hexName, searchFrom);
            if (hexPos == std::string_view::npos)
                return {};

            auto afterHex = hexPos + hexName.size();
            bool atBoundary = (afterHex == filename.size() || filename[afterHex] == '_');
            if (!atBoundary) {
                searchFrom = hexPos + 1;
                continue;
            }

            auto firstUnderscore = filename.find('_');
            if (firstUnderscore == std::string_view::npos || firstUnderscore >= hexPos)
                return {};

            auto start = firstUnderscore + 1;
            auto len   = afterHex - start;
            return std::string(filename.substr(start, len));
        }
        return {};
    }

    // Scan saves folder for any .ess containing the hex-encoded player name.
    std::string FindCharacterID(std::string_view hexName) {
        auto dir = GetSavesDirectory();
        if (dir.empty() || !fs::exists(dir))
            return {};

        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".ess")
                continue;
            auto id = ExtractCharacterID(entry.path().stem().string(), hexName);
            if (!id.empty())
                return id;
        }
        return {};
    }

    // Derive the game root from the main executable's location.
    fs::path GetGameRoot() {
        return fs::path(REL::Module::get().filePath()).parent_path();
    }

    // Write Data/SSSOConf/{name}.txt with the character ID.
    // If the file already contains the correct ID it is left alone.
    // If the file is missing, contains "notset", or has a stale/wrong
    // ID, it is (re-)created with the current ID.
    void WriteConfigFile(std::string_view playerName, std::string_view characterID) {
        auto configDir = GetGameRoot() / "Data" / "SSSOConf";

        if (!fs::exists(configDir)) {
            std::error_code ec;
            fs::create_directories(configDir, ec);
            if (ec) {
                SKSE::log::error("Cannot create SSSOConf dir: {}", ec.message());
                return;
            }
        }

        auto wideName = NarrowToWide(playerName);
        if (wideName.empty()) {
            SKSE::log::error("Cannot convert player name '{}' to wide string", playerName);
            return;
        }
        auto path = configDir / (wideName + L".txt");

        // Check whether the existing file already has the right ID.
        if (fs::exists(path)) {
            std::ifstream in(path);
            std::string buf((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
            if (buf.find(characterID) != std::string::npos) {
                SKSE::log::trace("Config for '{}' already up-to-date", playerName);
                return;
            }
            // Stale, wrong, or "notset" — will be overwritten below.
            SKSE::log::info("Updating stale config for '{}'", playerName);
        }

        std::ofstream out(path, std::ios::trunc);
        if (!out) {
            SKSE::log::error("Cannot write config for '{}'", playerName);
            return;
        }

        out << "{\n\n    \"information\" : {\n        \"ID\" : \""
            << characterID
            << "\"\n    }\n}";

        SKSE::log::info("Wrote SSSO config: '{}' -> '{}'", playerName, characterID);
    }

    // Core logic: look up current player's character ID and write the config.
    void AutoConfig() {
        const auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        const char* name = player->GetName();
        if (!name || !*name) return;

        std::string playerName(name);
        auto hexName     = NameToHex(playerName);
        auto characterID = FindCharacterID(hexName);

        if (characterID.empty()) {
            SKSE::log::warn("No saves found for '{}' (hex {})", playerName, hexName);
            return;
        }

        WriteConfigFile(playerName, characterID);
    }

    // kSaveGame delivers the save file path — parse it directly so brand-new
    // characters get configured on their very first save.
    void OnSaveGame(const void* data, [[maybe_unused]] std::uint32_t dataLen) {
        if (!data) return;

        const auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) return;

        const char* name = player->GetName();
        if (!name || !*name) return;

        std::string playerName(name);
        auto hexName = NameToHex(playerName);

        const auto* raw = static_cast<const char*>(data);
        std::string savePath(raw, strnlen(raw, dataLen ? dataLen : 4096));
        auto filename = fs::path(savePath).stem().string();
        auto characterID = ExtractCharacterID(filename, hexName);

        if (characterID.empty())
            characterID = FindCharacterID(hexName);

        if (!characterID.empty())
            WriteConfigFile(playerName, characterID);
    }

    void MessageCallback(SKSE::MessagingInterface::Message* msg) {
        switch (msg->type) {
        case SKSE::MessagingInterface::kPostLoadGame:
            // data is the success bool cast directly to void*, NOT a bool*.
            if (reinterpret_cast<std::uintptr_t>(msg->data) != 0)
                AutoConfig();
            break;
        case SKSE::MessagingInterface::kSaveGame:
            OnSaveGame(msg->data, msg->dataLen);
            break;
        }
    }

    void InitLogging() {
        auto path = SKSE::log::log_directory();
        if (!path) return;

        *path /= "SSSOAutoConfig.log"sv;
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log  = std::make_shared<spdlog::logger>("SSSOAutoConfig"s, std::move(sink));
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(log));
    }
}

// --- SKSE plugin entry point (version info auto-generated by CMake) ---

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    InitLogging();
    SKSE::Init(a_skse);

    SKSE::log::info("SSSOAutoConfig v1.0.4 loaded ({})",
        REL::Module::IsVR() ? "VR" : "SSE");

    const auto* messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(MessageCallback)) {
        SKSE::log::critical("Failed to register SKSE message listener");
        return false;
    }

    return true;
}
