#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace nexora::core {

struct ProfileInfo {
    std::string id;
    std::string name;
    std::string avatar_icon;
    std::string profile_dir_path;
    bool is_guest = false;
    bool is_incognito = false;
};

class ProfileManager {
public:
    static ProfileManager& Instance() {
        static ProfileManager instance;
        return instance;
    }

    bool Init(const std::string& user_data_dir);

    std::vector<ProfileInfo> GetProfiles() const;
    ProfileInfo GetActiveProfile() const;
    bool SwitchProfile(const std::string& profile_id);

    ProfileInfo CreateProfile(const std::string& name, const std::string& avatar_icon = "avatar_default.png");
    bool DeleteProfile(const std::string& profile_id);

    ProfileInfo CreateIncognitoProfile();

private:
    ProfileManager();
    ~ProfileManager();

    void LoadProfiles();
    void SaveProfiles();

    std::string user_data_dir_;
    mutable std::mutex mutex_;
    std::vector<ProfileInfo> profiles_;
    std::string active_profile_id_ = "Default";
};

} // namespace nexora::core
