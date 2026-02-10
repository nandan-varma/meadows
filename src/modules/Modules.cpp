#include "Modules.h"
#include <algorithm>

namespace meadows {

ModuleName::ModuleName(const std::string &name) {
  size_t start = 0;
  while (start < name.size()) {
    size_t pos = name.find('.', start);
    if (pos == std::string::npos) {
      components.push_back(name.substr(start));
      break;
    }
    components.push_back(name.substr(start, pos - start));
    start = pos + 1;
  }
}

ModuleName::ModuleName(std::vector<std::string> comps)
    : components(std::move(comps)) {}

std::string ModuleName::toString() const {
  std::string result;
  for (size_t i = 0; i < components.size(); ++i) {
    if (i > 0)
      result += ".";
    result += components[i];
  }
  return result;
}

std::string ModuleName::toPath() const {
  std::string result;
  for (size_t i = 0; i < components.size(); ++i) {
    if (i > 0)
      result += "/";
    result += components[i];
  }
  return result;
}

ModuleName ModuleName::parent() const {
  if (components.empty())
    return ModuleName();
  ModuleName parent;
  parent.components =
      std::vector<std::string>(components.begin(), components.end() - 1);
  return parent;
}

ModuleName ModuleName::append(const std::string &component) const {
  ModuleName result = *this;
  result.components.push_back(component);
  return result;
}

bool ModuleName::operator==(const ModuleName &other) const {
  return components == other.components;
}

bool ModuleName::operator<(const ModuleName &other) const {
  return toString() < other.toString();
}

bool ModuleName::isSubModuleOf(const ModuleName &other) const {
  if (components.size() <= other.components.size())
    return false;
  for (size_t i = 0; i < other.components.size(); ++i) {
    if (components[i] != other.components[i])
      return false;
  }
  return true;
}

bool ModuleName::startsWith(const ModuleName &other) const {
  if (other.components.size() > components.size())
    return false;
  for (size_t i = 0; i < other.components.size(); ++i) {
    if (components[i] != other.components[i])
      return false;
  }
  return true;
}

ModuleName ModuleName::fromPath(const std::string &path) {
  ModuleName name;
  size_t start = 0;
  while (start < path.size()) {
    size_t pos = path.find('/', start);
    if (pos == std::string::npos) {
      std::string ext = path.substr(start);
      // Remove .ms extension if present
      if (ext.size() > 3 && ext.substr(ext.size() - 3) == ".ms") {
        ext = ext.substr(0, ext.size() - 3);
      }
      name.components.push_back(ext);
      break;
    }
    std::string component = path.substr(start, pos - start);
    if (component.size() > 3 &&
        component.substr(component.size() - 3) == ".ms") {
      component = component.substr(0, component.size() - 3);
    }
    name.components.push_back(component);
    start = pos + 1;
  }
  return name;
}

bool ImportStatement::importsAll() const { return specificImports.empty(); }

bool ImportStatement::hasAlias() const { return !alias.empty(); }

} // namespace meadows
