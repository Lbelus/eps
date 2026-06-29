#include <rest_api/config_loader.hpp>

int find_ch(char* str, char ch)
{
    int index = 0;
    while (str[index] != ch && str[index] != '\0')
    {
      index += 1;
    }
    return index;
}

void to_lower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char ch)
        { 
            return static_cast<char>(std::tolower(ch));
        });
}

static std::string trim(const std::string& input)
{
    const std::string whitespace = " \t\r\n";

    const std::size_t start = input.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return "";

    const std::size_t end = input.find_last_not_of(whitespace);

    return input.substr(start, end - start + 1);
}

static std::string strip_comment(const std::string& line)
{
    const std::size_t pos = line.find('#');

    if (pos == std::string::npos)
        return line;

    return line.substr(0, pos);
}

static std::size_t count_leading_spaces(const std::string& line)
{
    std::size_t count = 0;

    for (char c : line)
    {
        if (c == ' ')
        {
            ++count;
        }
        else if (c == '\t')
        {
            throw std::runtime_error("Tabs are not allowed for indentation");
        }
        else
        {
            break;
        }
    }

    return count;
}

static unsigned int parse_uint(const std::string& key, const std::string& value)
{
    try
    {
        std::size_t parsed = 0;
        unsigned long result = std::stoul(value, &parsed, 10);

        if (parsed != value.size())
            throw std::runtime_error("Invalid trailing characters");

        return static_cast<unsigned int>(result);
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Invalid unsigned integer for key '" + key + "': " + value);
    }
}


std::vector<std::string> split_fc(const std::string& str, char ch)
{
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = str.find(ch, pos)) != std::string::npos)
    {
        std::string substring(str.substr(prev_pos, pos - prev_pos));
        substring.erase(std::remove(substring.begin(), substring.end(), ' '), substring.end());
        output.push_back(substring);
        prev_pos = ++pos;
    }
    output.push_back(str.substr(prev_pos, pos-prev_pos)); // Last word
    return output;
}

// void print_tokens(const std::vector<std::string>& vec)
// {

//     std::cout << "key:" << vec.front();
//     std::cout << "; value:" << vec.back() << std::endl;
// }



string_code map_string(const std::string& str)
{
    if (str == "db_type")
        return string_code::db_type;

    if (str == "host")
        return string_code::host;

    if (str == "port")
        return string_code::port;

    if (str == "user")
        return string_code::user;

    if (str == "password")
        return string_code::password;

    if (str == "database")
        return string_code::database;

    if (str == "pool_size")
        return string_code::pool_size;

    if (str == "threads")
        return string_code::threads;

    if (str == "allowed_origins")
        return string_code::allowed_origins;

    if (str == "bind_addr")
        return string_code::bind_addr;

    return string_code::unknown;
}

std::string get_required_env(const std::string& name)
{
    const char* value = std::getenv(name.c_str());

    if (value == nullptr || value[0] == '\0')
    {
        throw std::runtime_error("Missing required environment variable: " + name);
    }

    return std::string(value);
}

DbType map_db_string(std::string db_type)
{
    to_lower(db_type);
    if (db_type == "mysql") 
        return DbType::MySql;

    if (db_type == "redis")
        return DbType::Redis;

    if (db_type == "PostgreSQL"
        return DbType::PostgreSQL;
}

void parser(app_config_t& config, const std::string& section, const std::vector<std::string> vec)
{   
    if (vec.size() < 2)
    {
        throw std::runtime_error("Invalid config pair: expected key/value");
    }
    const std::string key = trim(vec.front());
    const std::string value = trim(vec.back());

    if (section == "server")
    {
        switch (map_string(key))
        {
            case string_code::host:
                config.server_config.host = value;
                break;

            case string_code::bind_addr:
                config.server_config.bind_addr = value;
                break;

            case string_code::port:
                config.server_config.port = static_cast<unsigned short>(
                    parse_uint("server_config.port", value)
                );
                break;

            case string_code::threads:
                config.server_config.threads = parse_uint("server.threads", value);
                break;

            case string_code::allowed_origins:
                config.server_config.allowed_origins = value;
                break;

            default:
                throw std::runtime_error("Unknown server config key: " + key);
        }

        return;
    }

    if (section == "db")
    {
        switch (map_string(key))
        {
            case string_code::db_type:
                config.db_config.db_type = map_db_string(value);
                break;

            case string_code::host:
                config.db_config.host = value;
                break;

            case string_code::port:
                config.db_config.port = parse_uint("mysql.port", value);
                break;

            case string_code::user:
                config.db_config.user = value;
                break;

            case string_code::password:
                config.db_config.password = value;
                config.db_config.password = get_required_env(config.mysql.password);
                break;

            case string_code::database:
                config.db_config.database = value;
                break;

            case string_code::pool_size:
                config.db_config.pool_size = parse_uint("mysql.pool_size", value);
                break;

            default:
                throw std::runtime_error("Unknown db_config config key: " + key);
        }

        return;
    }
    throw std::runtime_error("Unknown config section: " + section);
}

app_config_t load_config(const char* filename)
{
    std::fstream config_file;
    config_file.open(filename);
    std::string line;
    std::vector<std::string> vec;
    std::string current_section;
    app_config_t config;
    std::cout << "LOADING CONFIGURATION FILE:" << std::endl;
    while (getline(config_file, line))
    {
        line = strip_comment(line);
        std::string clean_line = trim(line);
        if (line.empty())
        {
            continue;
        }
        std::size_t indent = count_leading_spaces(line);
        const std::size_t colon = clean_line.find(':');
        if (colon == std::string::npos)
        {
            continue;
        }
        vec.clear();
        vec.push_back(trim(clean_line.substr(0, colon)));
        vec.push_back(trim(clean_line.substr(colon + 1)));
        if (indent == 0)
        {
            current_section = vec.front();
        }
        else
        {
            std::cout << "loading " << current_section << " variables" << std::endl;
            parser(config, current_section, vec);
        }
    }
    std::cout << "success... proceding to boot rest api server_config..." << std::endl;
    return config;
}
