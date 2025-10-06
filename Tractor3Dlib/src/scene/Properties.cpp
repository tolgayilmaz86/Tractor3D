#include "pch.h"

#include "scene/Properties.h"

#include "framework/FileSystem.h"
#include "math/Quaternion.h"

namespace
{
using namespace tractor;

struct NamespaceInfo
{
    std::string name{};
    std::string id{};
    std::string parentID{};
    bool hasOpenBrace{ false };
    bool hasCloseBrace{ false };
};

/**
 * Reads the next character from the stream. Returns EOF if the end of the stream is reached.
 */
signed char readChar(Stream* stream)
{
    if (stream->eof()) return EOF;
    signed char c;
    if (stream->read(&c, 1, 1) != 1) return EOF;
    return c;
};

//-----------------------------------------------------------------------------
bool isCommentLine(const std::string& line)
{
    return line.size() >= 2 && line.substr(0, 2) == "//";
};

//-----------------------------------------------------------------------------
static bool isVariable(const std::string& str, std::string& outName)
{
    // Check if the string matches the variable pattern "${...}"
    if (str.size() > 3 && str.front() == '$' && str[1] == '{' && str.back() == '}')
    {
        // Extract the variable name (excluding "${" and "}")
        outName = std::string(str.substr(2, str.size() - 3));
        return true;
    }

    return false;
};

//-----------------------------------------------------------------------------
static std::string getVariableName(const std::string& str)
{
    // Check if the string matches the variable pattern "${...}"
    if (str.size() > 3 && str.front() == '$' && str[1] == '{' && str.back() == '}')
    {
        // Extract the variable name (excluding "${" and "}")
        return std::string(str.substr(2, str.size() - 3));
    }

    return str;
};

//-----------------------------------------------------------------------------
bool containsEquals(const std::string& line) { return line.find('=') != std::string::npos; };

//-----------------------------------------------------------------------------
std::string& trimWhiteSpace(std::string& str)
{
    auto start = str.find_first_not_of(" \t\n\r");
    auto end = str.find_last_not_of(" \t\n\r");
    return str = (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
};

//-----------------------------------------------------------------------------
std::optional<NamespaceInfo> parseNamespaceTokens(std::string& line, const std::string& trimmedLine)
{
    NamespaceInfo info;
    info.hasOpenBrace = line.find('{') != std::string::npos;
    info.hasCloseBrace = line.find('}') != std::string::npos;
    const bool hasInheritance = line.find(':') != std::string::npos;

    // Create a copy of the line to work with since we need to preserve the original
    std::string workingLine = line;

    // Helper lambda to split string by delimiters and return the next token
    auto getNextToken = [](std::string& str, const std::string& delimiters) -> std::string
    {
        if (str.empty()) return "";

        size_t start = str.find_first_not_of(delimiters);
        if (start == std::string::npos)
        {
            str.clear();
            return "";
        }

        size_t end = str.find_first_of(delimiters, start);
        std::string token = str.substr(start, end - start);

        if (end == std::string::npos)
        {
            str.clear();
        }
        else
        {
            str = str.substr(end);
        }

        return token;
    };

    // Parse name - split by whitespace and '{'
    std::string nameToken = getNextToken(workingLine, " \t\n{");
    if (nameToken.empty())
    {
        GP_ERROR("Error parsing properties file: failed to determine a valid token for line '%s'.",
                 line.c_str());
        return std::nullopt;
    }
    info.name = trimWhiteSpace(nameToken);

    // Parse ID - split by ':' and '{'
    std::string idToken = getNextToken(workingLine, ":{");
    if (!idToken.empty()) info.id = trimWhiteSpace(idToken);

    // Parse parent ID if inheritance is specified
    if (hasInheritance)
    {
        std::string parentToken = getNextToken(workingLine, "{");
        if (!parentToken.empty())
        {
            parentToken = parentToken.substr(parentToken.find(":") + 1); // ensure we start with ':'
            info.parentID = trimWhiteSpace(parentToken); // omit leading ':' inheritance character
        }
    }

    return info;
}

//-----------------------------------------------------------------------------
std::optional<std::pair<std::string, std::string>> parsePropertyTokens(std::string& line)
{
    // Find the first '=' character
    size_t equalsPos = line.find('=');
    if (equalsPos == std::string::npos)
    {
        GP_ERROR("Error parsing properties file: attribute without name.");
        return std::nullopt;
    }

    // Extract name (everything before '=')
    std::string name = line.substr(0, equalsPos);

    // Extract value (everything after '=')
    std::string value;
    if (equalsPos + 1 <= line.length())
    {
        value = line.substr(equalsPos + 1);
    }
    else
    {
        // No value after '=' - this is an error
        GP_ERROR("Error parsing properties file: attribute with name ('%s') but no value.",
                 name.c_str());
        return std::nullopt;
    }

    return std::make_pair(trimWhiteSpace(name), trimWhiteSpace(value));
}

/// @brief Class responsible for parsing properties from a stream into a Properties object.
class PropertiesParser
{
  public:
    PropertiesParser(Stream* stream, Properties* properties, int nestingDepth = 0)
        : m_stream(stream), m_properties(properties), m_nestingDepth(nestingDepth)
    {
    }

    void parse()
    {
        char line[LINE_BUFFER_SIZE];

        while (!m_stream->eof())
        {
            skipWhiteSpace(m_stream);

            if (m_stream->eof()) break;

            const auto lineResult = m_stream->readLine(line, LINE_BUFFER_SIZE);
            if (!lineResult)
            {
                GP_ERROR("Error reading line from file.");
                return;
            }

            // If we're parsing a sub-namespace and we encounter a closing brace at depth 0,
            // we should exit this parsing context
            if (m_nestingDepth > 0 && shouldExitNamespace(line))
            {
                return;
            }
            processLine(line);
        }
    }

  private:
    //-----------------------------------------------------------------------------
    void processLine(const std::string& line)
    {
        std::string lineStr(line);
        std::string trimmedLine = trimWhiteSpace(lineStr);

        if (handleComments(line, trimmedLine)) return;

        if (isCommentLine(line)) return;

        if (containsEquals(line))
            processPropertyLine(lineStr);
        else
            processNamespaceLine(lineStr, trimmedLine);
    }

    //-----------------------------------------------------------------------------
    bool handleComments(const std::string& line, const std::string& trimmedLine)
    {
        if (m_inComment)
        {
            // Check if the line starts with "*/" or the trimmed line ends with "*/"
            if (line.size() >= 2 && line.substr(0, 2) == "*/"
                || (trimmedLine.size() >= 2 && trimmedLine.substr(trimmedLine.size() - 2) == "*/"))
            {
                m_inComment = false;
            }
            return true;
        }

        // Check if the line starts with "/*"
        if (line.size() >= 2 && line.substr(0, 2) == "/*")
        {
            m_inComment = true;
            return true;
        }

        return false;
    }

    //-----------------------------------------------------------------------------
    void processPropertyLine(std::string& line)
    {
        const auto tokens = parsePropertyTokens(line);
        if (!tokens) return;

        const auto& [name, value] = *tokens;

        std::string variable;

        // first resolve value variable
        if (isVariable(value, variable))
        {
            auto itr = m_properties->_resolvedVariableMap.find(value);
            if (itr != m_properties->_resolvedVariableMap.end())
            {
                // Found a variable reference, replace value
                const auto& normalizedName = getVariableName(name);
                m_properties->setVariable(normalizedName, itr->second);

                if (isVariable(name, variable))
                    m_properties->_resolvedVariableMap.insert({ name, itr->second });
                return;
            }
        }

        if (isVariable(name, variable))
        {
            // Check if this variable assigned as another variable
            // e.g.: ${normalColor} = #ffffffff
            //       ${focusColor} = ${normalColor}
            //	In this case we want to resolve the value of ${normalColor} first
            m_properties->_resolvedVariableMap.insert({ name, value });
            m_properties->setVariable(variable, value);
        }
        else
        {
            m_properties->addProperty(name, value);
        }
    }

    //-----------------------------------------------------------------------------
    void skipWhiteSpace(Stream* stream)
    {
        signed char c;
        do
        {
            c = readChar(stream);
        } while (isspace(c) && c != EOF);

        // If we are not at the end of the file, then since we found a
        // non-whitespace character, we put the cursor back in front of it.
        if (c != EOF)
            if (stream->seek(-1, SEEK_CUR) == false)
                GP_ERROR("Failed to seek backwards one character after skipping whitespace.");
    }

    //-----------------------------------------------------------------------------
    void processNamespaceLine(std::string& line, const std::string& trimmedLine)
    {
        const auto namespaceInfo = parseNamespaceTokens(line, trimmedLine);
        if (!namespaceInfo) return;

        const auto& [name, id, parentID, hasOpenBrace, hasCloseBrace] = *namespaceInfo;

        if (name[0] == '}')
        {
            return; // End of namespace
        }

        const bool isInlineNamespace = hasOpenBrace && hasCloseBrace;

        if (isInlineNamespace)
            handleInlineNamespace(name, id, parentID);
        else if (hasOpenBrace)
            handleOpenNamespace(name, id, parentID);
        else
            handleDeferredNamespace(line, name, id, parentID);
    }

    //-----------------------------------------------------------------------------
    void handleInlineNamespace(const std::string& name,
                               const std::string& id,
                               const std::string& parentID)
    {
        seekBeforeClosingBrace();
        createNamespace(name, id, parentID);
        seekAfterClosingBrace();
    }

    //-----------------------------------------------------------------------------
    void handleOpenNamespace(const std::string& name, const std::string& id, const std::string& parentID)
    {
        createNamespace(name, id, parentID);
    }

    //-----------------------------------------------------------------------------
    void handleDeferredNamespace(std::string& line,
                                 const std::string& name,
                                 const std::string& id,
                                 const std::string& parentID)
    {
        skipWhiteSpace(m_stream);
        const int nextChar = readChar(m_stream);

        if (nextChar == '{')
        {
            createNamespace(name, id, parentID);
        }
        else
        {
            // Back up and treat as property
            if (m_stream->seek(-1, SEEK_CUR) == false)
                GP_ERROR("Failed to seek backwards a single character after testing if the next "
                         "line starts with '{'.");

            const std::string value = id.empty() ? "" : id;
            m_properties->addProperty(name, value);
        }
    }

    bool shouldExitNamespace(const std::string& line)
    {
        std::string trimmedLine = line;
        trimWhiteSpace(trimmedLine);

        // Check if this line is just a closing brace
        return trimmedLine == "}";
    }

    //-----------------------------------------------------------------------------
    void createNamespace(const std::string& name, const std::string& id, const std::string& parentID)
    {
        m_properties->addNamespace(m_stream, name, id, parentID, m_properties, m_nestingDepth + 1);
    }

    //-----------------------------------------------------------------------------
    void seekBeforeClosingBrace()
    {
        if (m_stream->seek(-1, SEEK_CUR) == false)
        {
            GP_ERROR("Failed to seek back to before a '}' character in properties file.");
            return;
        }

        while (readChar(m_stream) != '}')
        {
            if (m_stream->seek(-2, SEEK_CUR) == false)
            {
                GP_ERROR("Failed to seek back to before a '}' character in properties file.");
                return;
            }
        }

        if (m_stream->seek(-1, SEEK_CUR) == false)
        {
            GP_ERROR("Failed to seek back to before a '}' character in properties file.");
        }
    }

    void seekAfterClosingBrace()
    {
        if (m_stream->seek(1, SEEK_CUR) == false)
        {
            GP_ERROR("Failed to seek to immediately after a '}' character in properties file.");
        }
    }

  private:
    Stream* m_stream{ nullptr };
    Properties* m_properties{ nullptr };
    bool m_inComment{ false };
    int m_nestingDepth{ 0 }; // tracks brace depth
    static constexpr size_t LINE_BUFFER_SIZE = 2048;
};
} // namespace

namespace tractor
{

//-----------------------------------------------------------------------------
void calculateNamespacePath(const std::string& urlString,
                            std::string& fileString,
                            std::vector<std::string>& namespacePath)
{
    // If the url references a specific namespace within the file,
    // calculate the full namespace path to the final namespace.
    size_t loc = urlString.rfind("#");
    if (loc != std::string::npos)
    {
        fileString = urlString.substr(0, loc);
        std::string namespacePathString = urlString.substr(loc + 1);
        while ((loc = namespacePathString.find("/")) != std::string::npos)
        {
            namespacePath.push_back(namespacePathString.substr(0, loc));
            namespacePathString = namespacePathString.substr(loc + 1);
        }
        namespacePath.push_back(namespacePathString);
    }
    else
    {
        fileString = urlString;
    }
}

//-----------------------------------------------------------------------------
Properties* getPropertiesFromNamespacePath(Properties* properties,
                                           const std::vector<std::string>& namespacePath)
{
    // If the url references a specific namespace within the file,
    // return the specified namespace or notify the user if it cannot be found.
    if (namespacePath.size() > 0)
    {
        size_t size = namespacePath.size();
        properties->rewind();
        Properties* iter = properties->getNextNamespace();
        for (size_t i = 0; i < size;)
        {
            while (true)
            {
                if (iter == nullptr)
                {
                    GP_WARN("Failed to load properties object from url.");
                    return nullptr;
                }

                if (iter->getId() == namespacePath[i])
                {
                    if (i != size - 1)
                    {
                        properties = iter->getNextNamespace();
                        iter = properties;
                    }
                    else
                        properties = iter;

                    i++;
                    break;
                }

                iter = properties->getNextNamespace();
            }
        }

        return properties;
    }
    else
        return properties;
}

//-----------------------------------------------------------------------------
Properties::Properties(const Properties& copy)
    : _namespace(copy._namespace), _id(copy._id), _parentID(copy._parentID),
      _properties(copy._properties), _parent(copy._parent)
{
    setDirectoryPath(copy._dirPath);
    _namespaces = std::vector<Properties*>();
    std::vector<Properties*>::const_iterator it;
    for (const auto& ns : copy._namespaces)
    {
        assert(ns);
        _namespaces.emplace_back(new Properties(*ns));
    }
    rewind();
}

//-----------------------------------------------------------------------------
Properties::Properties(Stream* stream)
{
    // Start with nesting depth 0 for root level
    PropertiesParser parser(stream, this, 0);
    parser.parse();

    rewind();
}

//-----------------------------------------------------------------------------
Properties::Properties(Stream* stream,
                       const std::string& name,
                       const std::string& id,
                       const std::string& parentID,
                       Properties* parent,
                       int nestingDepth)
    : _namespace(name), _parent(parent)
{
    if (!id.empty())
    {
        _id = id;
    }

    if (!parentID.empty())
    {
        _parentID = parentID;
    }

    PropertiesParser parser(stream, this, nestingDepth);
    parser.parse();
    rewind();
}

//-----------------------------------------------------------------------------
Properties* Properties::create(const std::string& url)
{
    if (url.empty())
    {
        GP_ERROR("Attempting to create a Properties object from an empty URL!");
        return nullptr;
    }

    // Calculate the file and full namespace path from the specified url.
    std::string urlString = url;
    std::string fileString;
    std::vector<std::string> namespacePath;
    ::calculateNamespacePath(urlString, fileString, namespacePath);

    std::unique_ptr<Stream> stream(FileSystem::open(fileString));
    if (stream.get() == nullptr)
    {
        GP_WARN("Failed to open file '%s'.", fileString.c_str());
        return nullptr;
    }

    Properties* properties = new Properties(stream.get());
    properties->resolveInheritance();
    stream->close();

    properties->rewind();

    // Get the specified properties object.
    Properties* p = ::getPropertiesFromNamespacePath(properties, namespacePath);
    if (!p)
    {
        GP_WARN("Failed to load properties from url '%s'.", url);
        SAFE_DELETE(properties);
        return nullptr;
    }

    // If the loaded properties object is not the root namespace,
    // then we have to clone it and delete the root namespace
    // so that we don't leak memory.
    if (p != properties)
    {
        p = p->clone();
        SAFE_DELETE(properties);
    }
    p->setDirectoryPath(FileSystem::getDirectoryName(fileString));
    return p;
}

//-----------------------------------------------------------------------------
void Properties::readProperties(Stream* stream)
{
    assert(stream);

    bool isSubNamespace = (_parent != nullptr);
    PropertiesParser parser(stream, this, isSubNamespace);
    parser.parse();
}

//-----------------------------------------------------------------------------
Properties::~Properties()
{
    SAFE_DELETE(_dirPath);
    for (size_t i = 0, count = _namespaces.size(); i < count; ++i)
    {
        SAFE_DELETE(_namespaces[i]);
    }

    SAFE_DELETE(_variables);
}

//-----------------------------------------------------------------------------
void Properties::resolveInheritance(const std::string& id)
{
    // Namespaces can be defined like so:
    // "name id : parentID { }"
    // This method merges data from the parent namespace into the child.

    // Get a top-level namespace.
    Properties* derived;
    if (!id.empty())
    {
        derived = getNamespace(id);
    }
    else
    {
        derived = getNextNamespace();
    }
    while (derived)
    {
        // If the namespace has a parent ID, find the parent.
        if (!derived->_parentID.empty())
        {
            derived->_visited = true;
            Properties* parent = getNamespace(derived->_parentID);
            if (parent)
            {
                assert(!parent->_visited);
                resolveInheritance(parent->getId());

                // Copy the child.
                Properties* overrides = new Properties(*derived);

                // Delete the child's data.
                for (size_t i = 0, count = derived->_namespaces.size(); i < count; i++)
                {
                    SAFE_DELETE(derived->_namespaces[i]);
                }

                // Copy data from the parent into the child.
                derived->_properties = parent->_properties;
                derived->_namespaces.clear();
                derived->_namespaces.reserve(parent->_namespaces.size());

                std::ranges::transform(parent->_namespaces,
                                       std::back_inserter(derived->_namespaces),
                                       [](const auto& ns) { return new Properties(*ns); });

                derived->rewind();

                // Take the original copy of the child and override the data copied from the parent.
                derived->mergeWith(overrides);

                // Delete the child copy.
                SAFE_DELETE(overrides);
            }
            derived->_visited = false;
        }

        // Resolve inheritance within this namespace.
        derived->resolveInheritance();

        // Get the next top-level namespace and check again.
        if (id.empty())
        {
            derived = getNextNamespace();
        }
        else
        {
            derived = nullptr;
        }
    }
}

//-----------------------------------------------------------------------------
void Properties::mergeWith(Properties* overrides)
{
    assert(overrides);

    // Overwrite or add each property found in child.
    overrides->rewind();
    auto property = overrides->getNextProperty();
    while (property)
    {
        this->setString(property->name, overrides->getString());
        property = overrides->getNextProperty();
    }
    this->_propertiesItr = this->_properties.end();

    // Merge all common nested namespaces, add new ones.
    Properties* overridesNamespace = overrides->getNextNamespace();
    while (overridesNamespace)
    {
        bool merged = false;

        rewind();
        Properties* derivedNamespace = getNextNamespace();
        while (derivedNamespace)
        {
            if (derivedNamespace->getNamespace() == overridesNamespace->getNamespace()
                && derivedNamespace->getId() == overridesNamespace->getId())
            {
                derivedNamespace->mergeWith(overridesNamespace);
                merged = true;
            }

            derivedNamespace = getNextNamespace();
        }

        if (!merged)
        {
            // Add this new namespace.
            this->_namespaces.emplace_back(new Properties(*overridesNamespace));
            this->_namespacesItr = this->_namespaces.end();
        }

        overridesNamespace = overrides->getNextNamespace();
    }
}

//-----------------------------------------------------------------------------
Property* Properties::getNextProperty()
{
    if (_propertiesItr == _properties.end())
    {
        // Restart from the beginning
        _propertiesItr = _properties.begin();
    }
    else
    {
        // Move to the next
        ++_propertiesItr;
    }

    return _propertiesItr == _properties.end() ? nullptr : &(*_propertiesItr);
}

//-----------------------------------------------------------------------------
Properties* Properties::getNextNamespace()
{
    if (_namespacesItr == _namespaces.end())
    {
        // Restart from the beginning
        _namespacesItr = _namespaces.begin();

        // indicate finished
        return nullptr;
    }

    // Get current namespace and advance iterator
    Properties* ns = *_namespacesItr;

    // Move to the next
    ++_namespacesItr;

    return ns;
}

//-----------------------------------------------------------------------------
void Properties::rewind()
{
    _propertiesItr = _properties.end();
    _namespacesItr = _namespaces.begin();
}

//-----------------------------------------------------------------------------
Properties* Properties::getNamespace(const std::string& id, bool searchNames, bool recurse) const
{
    if (id == "base")
    {
        __nop();
    }

    for (auto* p : _namespaces)
    {
        const std::string& compareStr = searchNames ? p->_namespace : p->_id;
        if (compareStr == id)
        {
            return p;
        }

        if (recurse)
        {
            // Search recursively.
            Properties* childNamespace = p->getNamespace(id, searchNames, true);
            if (childNamespace)
            {
                return childNamespace;
            }
        }
    }

    return nullptr;
}

//-----------------------------------------------------------------------------
bool Properties::exists(const std::string& name) const
{
    if (name.empty()) return false;

    for (std::list<Property>::const_iterator itr = _properties.begin(); itr != _properties.end();
         ++itr)
    {
        if (itr->name == name) return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
static const bool isStringNumeric(const std::string& str)
{
    if (str.empty()) return false;

    auto it = str.begin();

    // Check for optional leading '-' sign
    if (*it == '-') ++it;

    // Ensure there's at least one digit after the sign
    if (it == str.end() || !std::isdigit(*it)) return false;

    // Check the rest of the string
    bool hasDecimalPoint = false;
    for (; it != str.end(); ++it)
    {
        if (*it == '.')
        {
            if (hasDecimalPoint) // Only one decimal point is allowed
                return false;
            hasDecimalPoint = true;
        }
        else if (!std::isdigit(*it))
        {
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
Properties::Type Properties::getType(const std::string& name) const
{
    const std::string value = getString(name);
    if (value.empty())
    {
        return Properties::NONE;
    }

    // Parse the value to determine the format
    size_t commaCount = std::count(value.begin(), value.end(), ',');

    switch (commaCount)
    {
        case 0:
            return isStringNumeric(value) ? Properties::NUMBER : Properties::STRING;
        case 1:
            return Properties::VECTOR2;
        case 2:
            return Properties::VECTOR3;
        case 3:
            return Properties::VECTOR4;
        case 15:
            return Properties::MATRIX;
        default:
            return Properties::STRING;
    }
}

//-----------------------------------------------------------------------------
const std::string& Properties::getString(const std::string& name, const std::string& defaultValue) const
{
    if (name.empty())
    {
        return _propertiesItr != _properties.end() ? _propertiesItr->value : defaultValue;
    }

    std::string variable;
    // If 'name' is a variable, return the variable value
    if (isVariable(name, variable))
    {
        return getVariable(variable, defaultValue);
    }

    auto it = std::find_if(_properties.begin(),
                           _properties.end(),
                           [&name](const Property& prop) { return prop.name == name; });

    if (it != _properties.end())
    {
        // If 'name' is a variable, return the variable value
        if (isVariable(it->value, variable))
        {
            return getVariable(variable, defaultValue);
        }
        return it->value;
    }

    return defaultValue;
}

//-----------------------------------------------------------------------------
bool Properties::setString(const std::string& name, const std::string& value)
{
    // If the name is empty, return false immediately
    if (name.empty())
    {
        // If there's no current property, return false
        if (_propertiesItr == _properties.end()) return false;

        // Update the current property
        _propertiesItr->value = value;

        return true;
    }

    // Check if the property already exists and update it
    auto it = std::find_if(_properties.begin(),
                           _properties.end(),
                           [&name](const Property& prop) { return prop.name == name; });

    if (it != _properties.end())
    {
        it->value = value;
        return true;
    }

    // Add a new property if no match is found
    _properties.emplace_back(name, value);

    return true;
}

//-----------------------------------------------------------------------------
bool Properties::getBool(const std::string& name, bool defaultValue) const
{
    const std::string& valueString = getString(name);
    if (!valueString.empty())
    {
        return valueString == "true";
    }

    return defaultValue;
}

//-----------------------------------------------------------------------------
int Properties::getInt(const std::string& name) const
{
    const std::string& valueString = getString(name);

    try
    {
        if (!valueString.empty())
        {
            return std::stoi(valueString);
        }
    }
    catch (const std::exception&)
    {
        GP_ERROR("Error attempting to parse property '%s' as an integer: invalid argument.",
                 name.c_str());
    }

    return 0;
}

//-----------------------------------------------------------------------------
float Properties::getFloat(const std::string& name) const
{
    const std::string& valueString = getString(name);
    try
    {
        return std::stof(valueString);
    }
    catch (const std::exception&)
    {
        return 0.0f;
    }

    return 0.0f;
}

//-----------------------------------------------------------------------------
long Properties::getLong(const std::string& name) const
{
    const std::string& valueString = getString(name);
    try
    {
        return std::stol(valueString);
    }
    catch (const std::exception&)
    {
        return 0L;
    }

    return 0L;
}

//-----------------------------------------------------------------------------
bool Properties::getMatrix(const std::string& name, Matrix* out) const
{
    assert(out);

    const std::string& valueString = getString(name);
    if (!valueString.empty())
    {
        float m[16];
        int scanned;
        scanned = sscanf(valueString.c_str(),
                         "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
                         &m[0],
                         &m[1],
                         &m[2],
                         &m[3],
                         &m[4],
                         &m[5],
                         &m[6],
                         &m[7],
                         &m[8],
                         &m[9],
                         &m[10],
                         &m[11],
                         &m[12],
                         &m[13],
                         &m[14],
                         &m[15]);

        if (scanned != 16)
        {
            GP_ERROR("Error attempting to parse property '%s' as a matrix.", name);
            out->setIdentity();
            return false;
        }

        out->set(m);
        return true;
    }

    out->setIdentity();
    return false;
}

//-----------------------------------------------------------------------------
bool Properties::getVector2(const std::string& name, Vector2* out) const
{
    return parseVector2(getString(name), out);
}

//-----------------------------------------------------------------------------
bool Properties::getVector3(const std::string& name, Vector3* out) const
{
    return parseVector3(getString(name), out);
}

//-----------------------------------------------------------------------------
bool Properties::getVector4(const std::string& name, Vector4* out) const
{
    return parseVector4(getString(name), out);
}

//-----------------------------------------------------------------------------
bool Properties::getQuaternionFromAxisAngle(const std::string& name, Quaternion* out) const
{
    return parseAxisAngle(getString(name), out);
}

//-----------------------------------------------------------------------------
bool Properties::getColor(const std::string& name, Vector3* out) const
{
    return parseColor(getString(name), out);
}

//-----------------------------------------------------------------------------
bool Properties::getColor(const std::string& name, Vector4* out) const
{
    return parseColor(getString(name), out);
}

//-----------------------------------------------------------------------------
bool Properties::getPath(const std::string& name, std::string* path) const
{
    const auto& valueString = getString(name);

    if (valueString.empty()) return false;

    if (FileSystem::fileExists(valueString))
    {
        path->assign(valueString);
        return true;
    }

    const Properties* prop = this;
    while (prop != nullptr)
    {
        // Search for the file path relative to the bundle file
        const std::string* dirPath = prop->_dirPath;
        if (dirPath != nullptr && !dirPath->empty())
        {
            std::string relativePath = *dirPath;
            relativePath.append(valueString);
            if (FileSystem::fileExists(relativePath))
            {
                path->assign(relativePath);
                return true;
            }
        }
        prop = prop->_parent;
    }

    // File not found
    return false;
}

//-----------------------------------------------------------------------------
const std::string& Properties::getVariable(const std::string& name,
                                           const std::string& defaultValue) const
{
    if (name.empty()) return defaultValue;

    // Search for variable in this Properties object
    if (_variables)
    {
        auto it = std::ranges::find_if(*_variables,
                                       [name](const Property& property)
                                       { return property.name == name; });

        if (it != _variables->end())
        {
            return it->value;
        }
    }

    // Search for variable in parent Properties
    return _parent ? _parent->getVariable(name, defaultValue) : defaultValue;
}

//-----------------------------------------------------------------------------
void Properties::setVariable(const std::string& name, const std::string& value)
{
    Property* prop = nullptr;

    // Search for variable in this Properties object and parents
    Properties* current = const_cast<Properties*>(this);
    while (current)
    {
        if (current->_variables)
        {
            for (size_t i = 0, count = current->_variables->size(); i < count; ++i)
            {
                Property* p = &(*current->_variables)[i];
                if (p->name == name)
                {
                    prop = p;
                    break;
                }
            }
        }
        current = current->_parent;
    }

    if (prop)
    {
        // Found an existing property, set it
        prop->value = value;
    }
    else
    {
        // Add a new variable with this name
        if (!_variables) _variables = new std::vector<Property>();
        _variables->emplace_back(Property(name, value));
    }
}

//-----------------------------------------------------------------------------
Properties* Properties::clone()
{
    Properties* p = new Properties();

    p->_namespace = _namespace;
    p->_id = _id;
    p->_parentID = _parentID;
    p->_properties = _properties;
    p->_propertiesItr = p->_properties.end();
    p->setDirectoryPath(_dirPath);

    for (size_t i = 0, count = _namespaces.size(); i < count; i++)
    {
        assert(_namespaces[i]);
        Properties* child = _namespaces[i]->clone();
        p->_namespaces.push_back(child);
        child->_parent = p;
    }
    p->_namespacesItr = p->_namespaces.end();

    return p;
}

//-----------------------------------------------------------------------------
void Properties::setDirectoryPath(const std::string* path)
{
    if (path)
    {
        setDirectoryPath(*path);
    }
    else
    {
        SAFE_DELETE(_dirPath);
    }
}

//-----------------------------------------------------------------------------
void Properties::setDirectoryPath(const std::string& path)
{
    if (_dirPath == nullptr)
    {
        _dirPath = new std::string(path);
    }
    else
    {
        _dirPath->assign(path);
    }
}

//-----------------------------------------------------------------------------
bool Properties::parseVector2(const std::string& str, Vector2* out)
{
    if (str.empty()) return false;

    float x, y;
    if (sscanf(str.c_str(), "%f,%f", &x, &y) == 2)
    {
        if (out) out->set(x, y);
        return true;
    }
    else
    {
        GP_WARN("Error attempting to parse property as a two-dimensional vector: %s", str);
    }

    if (out) out->set(0.0f, 0.0f);

    return false;
}

//-----------------------------------------------------------------------------
bool Properties::parseVector3(const std::string& str, Vector3* out)
{
    if (str.empty()) return false;

    float x, y, z;
    if (sscanf(str.c_str(), "%f,%f,%f", &x, &y, &z) == 3)
    {
        if (out) out->set(x, y, z);
        return true;
    }
    else
    {
        GP_WARN("Error attempting to parse property as a three-dimensional vector: %s", str);
    }

    if (out) out->set(0.0f, 0.0f, 0.0f);

    return false;
}

//-----------------------------------------------------------------------------
bool Properties::parseVector4(const std::string& str, Vector4* out)
{
    if (str.empty()) return false;

    float x, y, z, w;
    if (sscanf(str.c_str(), "%f,%f,%f,%f", &x, &y, &z, &w) == 4)
    {
        if (out) out->set(x, y, z, w);
        return true;
    }
    else
    {
        GP_WARN("Error attempting to parse property as a four-dimensional vector: %s", str);
    }

    if (out) out->set(0.0f, 0.0f, 0.0f, 0.0f);

    return false;
}

//-----------------------------------------------------------------------------
bool Properties::parseAxisAngle(const std::string& str, Quaternion* out)
{
    if (str.empty()) return false;

    float x, y, z, theta;
    if (sscanf(str.c_str(), "%f,%f,%f,%f", &x, &y, &z, &theta) == 4)
    {
        if (out) out->set(Vector3(x, y, z), MATH_DEG_TO_RAD(theta));
        return true;
    }
    else
    {
        GP_WARN("Error attempting to parse property as an axis-angle rotation: %s", str);
    }

    if (out) out->set(0.0f, 0.0f, 0.0f, 1.0f);

    return false;
}

//-----------------------------------------------------------------------------
bool Properties::parseColor(const std::string& str, Vector3* out)
{
    if (str.empty()) return false;

    if (str.length() == 7 && str[0] == '#')
    {
        // Read the string into an int as hex.
        unsigned int color;
        if (sscanf(str.c_str() + 1, "%x", &color) == 1)
        {
            if (out) out->set(Vector3::fromColor(color));
            return true;
        }
        else
        {
            // Invalid format
            GP_WARN("Error attempting to parse property as an RGB color: %s", str);
        }
    }
    else
    {
        // Not a color string.
        GP_WARN("Error attempting to parse property as an RGB color (not specified as a color "
                "string): %s",
                str);
    }

    if (out) out->set(0.0f, 0.0f, 0.0f);

    return false;
}

//-----------------------------------------------------------------------------
bool Properties::parseColor(const std::string& str, Vector4* out)
{
    if (str.empty()) return false;

    if (str.length() == 9 && str[0] == '#')
    {
        // Read the string into an int as hex.
        unsigned int color;
        if (sscanf(str.c_str() + 1, "%x", &color) == 1)
        {
            if (out) out->set(Vector4::fromColor(color));
            return true;
        }
        else
        {
            // Invalid format
            GP_WARN("Error attempting to parse property as an RGBA color: %s", str);
        }
    }
    else
    {
        // Not a color string.
        GP_WARN("Error attempting to parse property as an RGBA color (not specified as a color "
                "string): %s",
                str);
    }

    if (out) out->set(0.0f, 0.0f, 0.0f, 0.0f);

    return false;
}

//-----------------------------------------------------------------------------
void Properties::addProperty(const std::string& name, const std::string& value)
{
    _properties.emplace_back(Property(name, value));
}

//-----------------------------------------------------------------------------
void Properties::addNamespace(Stream* stream,
                              const std::string& name,
                              const std::string& id,
                              const std::string& parentID,
                              Properties* parent,
                              int nestingDepth)
{
    _namespaces.emplace_back(new Properties(stream, name, id, parentID, parent, nestingDepth));
}

} // namespace tractor
