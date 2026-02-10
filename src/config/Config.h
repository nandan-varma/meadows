/**
 * @file Config.h
 * @brief Configuration file parsing for Meadows projects.
 *
 * Parses meadows.toml configuration files to extract project metadata,
 * build settings, and dependencies.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace meadows {
namespace config {

/**
 * @brief Represents a dependency specification.
 */
struct Dependency {
  std::string name;
  std::string version;
  std::string gitUrl;
  std::string gitBranch;
  std::string path;
  std::string registry;
  bool isDev;

  Dependency() : isDev(false) {}

  bool isPathDependency() const { return !path.empty(); }
  bool isGitDependency() const { return !gitUrl.empty(); }
  bool isRegistryDependency() const {
    return !isPathDependency() && !isGitDependency();
  }
};

/**
 * @brief Project metadata section.
 */
struct ProjectConfig {
  std::string name;
  std::string version;
  std::string edition;
  std::string description;
  std::string authors;
  std::string license;
  std::string repository;
  std::string homepage;

  ProjectConfig()
      : name(""), version("0.1.0"), edition("2024"), description(""),
        authors(""), license("MIT"), repository(""), homepage("") {}
};

/**
 * @brief Build configuration section.
 */
struct BuildConfig {
  std::string target;
  int optLevel;
  bool debug;
  std::vector<std::string> flags;
  std::string outputDir;
  std::string entryPoint;

  BuildConfig()
      : target("native"), optLevel(2), debug(true), outputDir("build"),
        entryPoint("src/main.ms") {}
};

/**
 * @brief Complete configuration for a Meadows project.
 */
class Config {
public:
  Config();
  ~Config() = default;

  // Configuration sections
  ProjectConfig project;
  BuildConfig build;
  std::vector<Dependency> dependencies;
  std::vector<Dependency> devDependencies;

  // Extra metadata not in standard sections
  std::unordered_map<std::string, std::string> extras;

  /**
   * @brief Load configuration from a TOML file.
   * @param path Path to the TOML file.
   * @return true if successful, false otherwise.
   */
  bool loadFromFile(const std::string &path);

  /**
   * @brief Load configuration from a TOML string.
   * @param content TOML content as string.
   * @return true if successful, false otherwise.
   */
  bool loadFromString(const std::string &content);

  /**
   * @brief Find and load configuration from current directory.
   * Searches for meadows.toml or Meadows.toml in current directory.
   * @return true if found and loaded, false otherwise.
   */
  bool loadFromCurrentDirectory();

  /**
   * @brief Search for config file starting from a directory and going up.
   * @param startDir Directory to start searching from.
   * @return Path to config file, or empty string if not found.
   */
  static std::string findConfigFile(const std::string &startDir);

  /**
   * @brief Get the full path to the project root.
   * @return Absolute path to project directory containing config.
   */
  std::string getProjectRoot() const;

  /**
   * @brief Get a specific dependency by name.
   * @param name Dependency name.
   * @return Pointer to dependency, or nullptr if not found.
   */
  const Dependency *getDependency(const std::string &name) const;

  /**
   * @brief Validate the configuration.
   * @return Empty string if valid, error message otherwise.
   */
  std::string validate() const;

  /**
   * @brief Check if configuration is loaded.
   * @return true if a config file has been loaded.
   */
  bool isLoaded() const { return loaded_; }

  /**
   * @brief Get the path of the loaded config file.
   * @return Path to config file, or empty string if not loaded.
   */
  std::string getConfigPath() const { return configPath_; }

private:
  std::string configPath_;
  bool loaded_;

  void reset();
  bool parseTOML(const std::string &content);
  void parseProjectSection(
      const std::unordered_map<std::string, std::string> &table);
  void
  parseBuildSection(const std::unordered_map<std::string, std::string> &table);
  void parseDependenciesSection(
      const std::unordered_map<
          std::string, std::unordered_map<std::string, std::string>> &table,
      bool isDev);
};

/**
 * @brief Profile configuration for different build modes.
 */
struct BuildProfile {
  std::string name;
  int optLevel;
  bool debug;
  bool lto;
  std::vector<std::string> flags;

  static BuildProfile debugProfile();
  static BuildProfile releaseProfile();
  static BuildProfile testProfile();
};

/**
 * @brief Lock file for reproducible builds.
 */
class LockFile {
public:
  struct LockedDependency {
    std::string name;
    std::string version;
    std::string source;
    std::string checksum;
  };

  std::vector<LockedDependency> dependencies;

  bool load(const std::string &path);
  bool save(const std::string &path) const;
  bool exists(const std::string &path) const;
};

} // namespace config
} // namespace meadows

#endif // CONFIG_H
