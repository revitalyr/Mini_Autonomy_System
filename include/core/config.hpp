#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>

class Config {
private:
    std::map<std::string, std::string> config_map_;
    
public:
    Config() = default;
    
    // Load configuration from file
    bool load_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Parse key=value pairs
            size_t delimiter_pos = line.find('=');
            if (delimiter_pos != std::string::npos) {
                std::string key = line.substr(0, delimiter_pos);
                std::string value = line.substr(delimiter_pos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                config_map_[key] = value;
            }
        }
        
        return true;
    }
    
    // Get configuration values
    std::string get_string(const std::string& key, const std::string& default_value = "") const {
        auto it = config_map_.find(key);
        return (it != config_map_.end()) ? it->second : default_value;
    }
    
    int get_int(const std::string& key, int default_value = 0) const {
        auto it = config_map_.find(key);
        if (it != config_map_.end()) {
            try {
                return std::stoi(it->second);
            } catch (const std::exception&) {
                return default_value;
            }
        }
        return default_value;
    }
    
    float get_float(const std::string& key, float default_value = 0.0f) const {
        auto it = config_map_.find(key);
        if (it != config_map_.end()) {
            try {
                return std::stof(it->second);
            } catch (const std::exception&) {
                return default_value;
            }
        }
        return default_value;
    }
    
    bool get_bool(const std::string& key, bool default_value = false) const {
        auto it = config_map_.find(key);
        if (it != config_map_.end()) {
            std::string value = it->second;
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            return (value == "true" || value == "1" || value == "yes" || value == "on");
        }
        return default_value;
    }
    
    // Set configuration values
    void set_string(const std::string& key, const std::string& value) {
        config_map_[key] = value;
    }
    
    void set_int(const std::string& key, int value) {
        config_map_[key] = std::to_string(value);
    }
    
    void set_float(const std::string& key, float value) {
        config_map_[key] = std::to_string(value);
    }
    
    void set_bool(const std::string& key, bool value) {
        config_map_[key] = value ? "true" : "false";
    }
    
    // Save configuration to file
    bool save_to_file(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        file << "# Mini Autonomy System Configuration\n";
        file << "# Generated automatically\n\n";
        
        for (const auto& [key, value] : config_map_) {
            file << key << " = " << value << "\n";
        }
        
        return true;
    }
    
    // Check if key exists
    bool has_key(const std::string& key) const {
        return config_map_.find(key) != config_map_.end();
    }
    
    // Print all configuration
    void print_all() const {
        std::cout << "=== Configuration ===" << std::endl;
        for (const auto& [key, value] : config_map_) {
            std::cout << key << " = " << value << std::endl;
        }
        std::cout << "=====================" << std::endl;
    }
};
