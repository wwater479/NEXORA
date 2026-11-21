#include "profile_manager.h"
#include "../util/json_util.h"
#include "../util/path_util.h"
#include "../util/logger.h"
#include <fstream>
#include <algorithm>
#include <chrono>

namespace nexora::core {

using namespace nexora::util;

ProfileManager::ProfileManager() {}
ProfileManager::~ProfileManager() {}

bool ProfileManager::Init(const std::string& user_data_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    user_data_dir_ = user_data_dir.empty() ? PathUtil::GetUserDataDir() : user_data_dir;
    LoadProfiles();
    return true;
}

void ProfileManager::LoadProfiles() {
    profiles_.clear();
    std::string profiles_json_path = PathUtil::Combine(user_data_dir_, "profiles.json");

    if (PathUtil::FileExists(profiles_json_path)) {
        std::ifstream file(profiles_json_path);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            JsonValue root = JsonValue::Parse(content);
            if (root.IsObject()) {
                active_profile_id_ = root.Get("active_profile_id").AsString("Default");
                JsonValue arr = root.Get("profiles");
                if (arr.IsArray()) {
                    for (const auto& item : arr.AsArray()) {
                        if (!item.IsObject()) continue;
                        ProfileInfo info;
                        info.id = item.Get("id").AsString("Default");
                        info.name = item.Get("name").AsString("Person 1");
                        info.avatar_icon = item.Get("avatar_icon").AsString("avatar_default.png");
                        info.profile_dir_path = PathUtil::Combine(user_data_dir_, info.id);
                        info.is_guest = item.Get("is_guest").AsBool(false);
                        info.is_incognito = item.Get("is_incognito").AsBool(false);
                        profiles_.push_back(info);
                    }
                }
            }
        }
    }

    if (profiles_.empty()) {
        ProfileInfo default_profile;
        default_profile.id = "Default";
        default_profile.name = "Person 1";
        default_profile.avatar_icon = "avatar_blue.png";
        default_profile.profile_dir_path = PathUtil::Combine(user_data_dir_, "Default");
        default_profile.is_guest = false;
        default_profile.is_incognito = false;
        profiles_.push_back(default_profile);
        active_profile_id_ = "Default";
        PathUtil::EnsureDirectoryExists(default_profile.profile_dir_path);
        SaveProfiles();
    }
}

void ProfileManager::SaveProfiles() {
    std::string profiles_json_path = PathUtil::Combine(user_data_dir_, "profiles.json");
    JsonValue root(JsonValue::ObjectType{});
    root.Set("active_profile_id", active_profile_id_);

    JsonValue::ArrayType arr;
    for (const auto& p : profiles_) {
        if (p.is_incognito) continue; // Ephemeral, don't persist
        JsonValue p_obj(JsonValue::ObjectType{});
        p_obj.Set("id", p.id);
        p_obj.Set("name", p.name);
        p_obj.Set("avatar_icon", p.avatar_icon);
        p_obj.Set("is_guest", p.is_guest);
        p_obj.Set("is_incognito", p.is_incognito);
        arr.push_back(p_obj);
    }
    root.Set("profiles", arr);

    PathUtil::EnsureDirectoryExists(user_data_dir_);
    std::ofstream file(profiles_json_path);
    if (file.is_open()) {
        file << root.ToString(true);
    }
}

std::vector<ProfileInfo> ProfileManager::GetProfiles() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return profiles_;
}

ProfileInfo ProfileManager::GetActiveProfile() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& p : profiles_) {
        if (p.id == active_profile_id_) return p;
    }
    if (!profiles_.empty()) return profiles_[0];
    return ProfileInfo{"Default", "Person 1", "avatar_blue.png", PathUtil::Combine(user_data_dir_, "Default"), false, false};
}

bool ProfileManager::SwitchProfile(const std::string& profile_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& p : profiles_) {
        if (p.id == profile_id) {
            active_profile_id_ = profile_id;
            SaveProfiles();
            LOG_INFO << "Switched to profile: " << p.name << " (" << p.id << ")";
            return true;
        }
    }
    return false;
}

ProfileInfo ProfileManager::CreateProfile(const std::string& name, const std::string& avatar_icon) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string id = "Profile_" + std::to_string(profiles_.size() + 1);
    ProfileInfo info;
    info.id = id;
    info.name = name.empty() ? ("Person " + std::to_string(profiles_.size() + 1)) : name;
    info.avatar_icon = avatar_icon;
    info.profile_dir_path = PathUtil::Combine(user_data_dir_, id);
    info.is_guest = false;
    info.is_incognito = false;

    PathUtil::EnsureDirectoryExists(info.profile_dir_path);
    profiles_.push_back(info);
    SaveProfiles();
    return info;
}

bool ProfileManager::DeleteProfile(const std::string& profile_id) {
    if (profile_id == "Default") return false;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(profiles_.begin(), profiles_.end(), [&profile_id](const ProfileInfo& p) {
        return p.id == profile_id;
    });
    if (it != profiles_.end()) {
        profiles_.erase(it, profiles_.end());
        if (active_profile_id_ == profile_id) {
            active_profile_id_ = "Default";
        }
        SaveProfiles();
        return true;
    }
    return false;
}

ProfileInfo ProfileManager::CreateIncognitoProfile() {
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    ProfileInfo info;
    info.id = "Incognito_" + std::to_string(ts);
    info.name = "Incognito";
    info.avatar_icon = "avatar_incognito.png";
    info.profile_dir_path = PathUtil::Combine(user_data_dir_, info.id);
    info.is_guest = false;
    info.is_incognito = true;
    PathUtil::EnsureDirectoryExists(info.profile_dir_path);
    return info;
}

} // namespace nexora::core
