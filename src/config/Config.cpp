#include "Config.h"
#include "TOMLParser.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

namespace meadows {
namespace config {

Config::Config() : loaded_(false) { reset(); }

void Config::reset() {
  project = ProjectConfig();
  build = BuildConfig();
  dependencies.clear();
  devDependencies.clear();
  extras.clear();
  loaded_ = false;
  configPath_.clear();
}

bool Config::loadFromFile(const std::string &path) {
  reset();

  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Error: Cannot open config file: " << path << std::endl;
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  bool result = loadFromString(buffer.str());
  if (result) {
    configPath_ = path;
  }
  return result;
}

bool Config::loadFromString(const std::string &content) {
  reset();

  TOMLParser parser;
  TOMLValue root = parser.parse(content);

  if (!parser.success()) {
    std::cerr << "Error parsing TOML: " << parser.getError() << std::endl;
    return false;
  }

  // Parse [project] section
  if (root.tableValue.find("project") != root.tableValue.end()) {
    const TOMLValue &proj = root.tableValue.at("project");
    if (proj.isTable()) {
      if (proj.tableValue.find("name") != proj.tableValue.end()) {
        project.name = proj.tableValue.at("name").asString();
      }
      if (proj.tableValue.find("version") != proj.tableValue.end()) {
        project.version = proj.tableValue.at("version").asString();
      }
      if (proj.tableValue.find("edition") != proj.tableValue.end()) {
        project.edition = proj.tableValue.at("edition").asString();
      }
      if (proj.tableValue.find("description") != proj.tableValue.end()) {
        project.description = proj.tableValue.at("description").asString();
      }
      if (proj.tableValue.find("authors") != proj.tableValue.end()) {
        project.authors = proj.tableValue.at("authors").asString();
      }
      if (proj.tableValue.find("license") != proj.tableValue.end()) {
        project.license = proj.tableValue.at("license").asString();
      }
      if (proj.tableValue.find("repository") != proj.tableValue.end()) {
        project.repository = proj.tableValue.at("repository").asString();
      }
      if (proj.tableValue.find("homepage") != proj.tableValue.end()) {
        project.homepage = proj.tableValue.at("homepage").asString();
      }
    }
  }

  // Parse [build] section
  if (root.tableValue.find("build") != root.tableValue.end()) {
    const TOMLValue &bld = root.tableValue.at("build");
    if (bld.isTable()) {
      if (bld.tableValue.find("target") != bld.tableValue.end()) {
        build.target = bld.tableValue.at("target").asString();
      }
      if (bld.tableValue.find("opt-level") != bld.tableValue.end()) {
        build.optLevel =
            static_cast<int>(bld.tableValue.at("opt-level").asInteger());
      }
      if (bld.tableValue.find("debug") != bld.tableValue.end()) {
        build.debug = bld.tableValue.at("debug").asBoolean();
      }
      if (bld.tableValue.find("output-dir") != bld.tableValue.end()) {
        build.outputDir = bld.tableValue.at("output-dir").asString();
      }
      if (bld.tableValue.find("entry-point") != bld.tableValue.end()) {
        build.entryPoint = bld.tableValue.at("entry-point").asString();
      }
      if (bld.tableValue.find("flags") != bld.tableValue.end()) {
        const TOMLValue &flags = bld.tableValue.at("flags");
        if (flags.isArray()) {
          for (const auto &flag : flags.arrayValue) {
            build.flags.push_back(flag.asString());
          }
        }
      }
    }
  }

  // Parse [dependencies] section
  if (root.tableValue.find("dependencies") != root.tableValue.end()) {
    const TOMLValue &deps = root.tableValue.at("dependencies");
    if (deps.isTable()) {
      for (const auto &pair : deps.tableValue) {
        Dependency dep;
        dep.name = pair.first;

        if (pair.second.isString()) {
          // Simple version: "package" = "1.0.0"
          dep.version = pair.second.asString();
          dep.registry = "default";
        } else if (pair.second.isTable()) {
          // Complex dependency with options
          const auto &tbl = pair.second.tableValue;

          if (tbl.find("version") != tbl.end()) {
            dep.version = tbl.at("version").asString();
          }
          if (tbl.find("git") != tbl.end()) {
            dep.gitUrl = tbl.at("git").asString();
          }
          if (tbl.find("branch") != tbl.end()) {
            dep.gitBranch = tbl.at("branch").asString();
          }
          if (tbl.find("path") != tbl.end()) {
            dep.path = tbl.at("path").asString();
          }
          if (tbl.find("registry") != tbl.end()) {
            dep.registry = tbl.at("registry").asString();
          }
        }

        dependencies.push_back(dep);
      }
    }
  }

  // Parse [dev-dependencies] section
  if (root.tableValue.find("dev-dependencies") != root.tableValue.end()) {
    const TOMLValue &deps = root.tableValue.at("dev-dependencies");
    if (deps.isTable()) {
      for (const auto &pair : deps.tableValue) {
        Dependency dep;
        dep.name = pair.first;
        dep.isDev = true;

        if (pair.second.isString()) {
          dep.version = pair.second.asString();
        } else if (pair.second.isTable()) {
          const auto &tbl = pair.second.tableValue;

          if (tbl.find("version") != tbl.end()) {
            dep.version = tbl.at("version").asString();
          }
          if (tbl.find("git") != tbl.end()) {
            dep.gitUrl = tbl.at("git").asString();
          }
          if (tbl.find("branch") != tbl.end()) {
            dep.gitBranch = tbl.at("branch").asString();
          }
          if (tbl.find("path") != tbl.end()) {
            dep.path = tbl.at("path").asString();
          }
        }

        devDependencies.push_back(dep);
      }
    }
  }

  // Store any extra top-level keys
  for (const auto &pair : root.tableValue) {
    if (pair.first != "project" && pair.first != "build" &&
        pair.first != "dependencies" && pair.first != "dev-dependencies") {
      extras[pair.first] = "<table>";
    }
  }

  loaded_ = true;
  return true;
}

bool Config::loadFromCurrentDirectory() {
  std::string cwd(1024, '\0');
  if (getcwd(&cwd[0], cwd.size()) == nullptr) {
    return false;
  }
  cwd = cwd.c_str(); // Remove trailing nulls

  std::string configPath = findConfigFile(cwd);
  if (configPath.empty()) {
    return false;
  }

  return loadFromFile(configPath);
}

std::string Config::findConfigFile(const std::string &startDir) {
  std::string currentDir = startDir;

  while (!currentDir.empty()) {
    // Try meadows.toml
    std::string path1 = currentDir + "/meadows.toml";
    std::ifstream f1(path1);
    if (f1.good()) {
      f1.close();
      return path1;
    }

    // Try Meadows.toml
    std::string path2 = currentDir + "/Meadows.toml";
    std::ifstream f2(path2);
    if (f2.good()) {
      f2.close();
      return path2;
    }

    // Go up one directory
    size_t pos = currentDir.find_last_of("/\\");
    if (pos == std::string::npos || pos == 0) {
      break;
    }
    currentDir = currentDir.substr(0, pos);
  }

  return "";
}

std::string Config::getProjectRoot() const {
  if (configPath_.empty()) {
    return "";
  }

  size_t pos = configPath_.find_last_of("/\\");
  if (pos == std::string::npos) {
    return ".";
  }

  return configPath_.substr(0, pos);
}

const Dependency *Config::getDependency(const std::string &name) const {
  for (const auto &dep : dependencies) {
    if (dep.name == name) {
      return &dep;
    }
  }

  for (const auto &dep : devDependencies) {
    if (dep.name == name) {
      return &dep;
    }
  }

  return nullptr;
}

std::string Config::validate() const {
  if (!loaded_) {
    return "No configuration loaded";
  }

  if (project.name.empty()) {
    return "Project name is required";
  }

  // Validate project name (alphanumeric, hyphens, underscores)
  for (char c : project.name) {
    if (!std::isalnum(c) && c != '-' && c != '_') {
      return "Invalid project name: only alphanumeric, hyphens, and "
             "underscores allowed";
    }
  }

  // Validate version format (simplified semver check)
  if (!project.version.empty()) {
    int dotCount = 0;
    for (char c : project.version) {
      if (c == '.') {
        dotCount++;
      } else if (!std::isdigit(c)) {
        // Allow pre-release identifiers like 1.0.0-alpha
        if (c != '-' && !std::isalnum(c)) {
          return "Invalid version format";
        }
      }
    }
    if (dotCount < 1 || dotCount > 2) {
      return "Version should be in format X.Y or X.Y.Z";
    }
  }

  return "";
}

BuildProfile BuildProfile::debugProfile() {
  BuildProfile p;
  p.name = "debug";
  p.optLevel = 0;
  p.debug = true;
  p.lto = false;
  return p;
}

BuildProfile BuildProfile::releaseProfile() {
  BuildProfile p;
  p.name = "release";
  p.optLevel = 3;
  p.debug = false;
  p.lto = true;
  return p;
}

BuildProfile BuildProfile::testProfile() {
  BuildProfile p;
  p.name = "test";
  p.optLevel = 0;
  p.debug = true;
  p.lto = false;
  p.flags = {"--test"};
  return p;
}

bool LockFile::load(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return false;
  }

  // Simple TOML parsing for lock file
  TOMLParser parser;
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  TOMLValue root = parser.parse(buffer.str());
  if (!parser.success()) {
    return false;
  }

  dependencies.clear();

  if (root.tableValue.find("package") != root.tableValue.end()) {
    const TOMLValue &packages = root.tableValue.at("package");
    if (packages.isArray()) {
      for (const auto &pkg : packages.arrayValue) {
        if (pkg.isTable()) {
          LockedDependency dep;
          const auto &tbl = pkg.tableValue;

          if (tbl.find("name") != tbl.end()) {
            dep.name = tbl.at("name").asString();
          }
          if (tbl.find("version") != tbl.end()) {
            dep.version = tbl.at("version").asString();
          }
          if (tbl.find("source") != tbl.end()) {
            dep.source = tbl.at("source").asString();
          }
          if (tbl.find("checksum") != tbl.end()) {
            dep.checksum = tbl.at("checksum").asString();
          }

          dependencies.push_back(dep);
        }
      }
    }
  }

  return true;
}

bool LockFile::save(const std::string &path) const {
  std::ofstream file(path);
  if (!file.is_open()) {
    return false;
  }

  file << "# This file is automatically generated by Meadows.\n";
  file << "# It is not intended for manual editing.\n\n";

  for (const auto &dep : dependencies) {
    file << "[[package]]\n";
    file << "name = \"" << dep.name << "\"\n";
    file << "version = \"" << dep.version << "\"\n";
    if (!dep.source.empty()) {
      file << "source = \"" << dep.source << "\"\n";
    }
    if (!dep.checksum.empty()) {
      file << "checksum = \"" << dep.checksum << "\"\n";
    }
    file << "\n";
  }

  file.close();
  return true;
}

bool LockFile::exists(const std::string &path) const {
  std::ifstream file(path);
  bool good = file.good();
  file.close();
  return good;
}

} // namespace config
} // namespace meadows
