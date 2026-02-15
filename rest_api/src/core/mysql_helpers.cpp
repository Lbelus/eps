#include <mysql_helpers.hpp>

//MySQL++ is a C++ wrapper around the MySQL C API (libmysqlclient).

// The MySQL C API ultimately calls mysql_real_connect(), which requires plaintext credentials.

// There is no built-in mechanism in MySQL++ to accept encrypted credentials or automatically decrypt them.

// Even if you hash or encrypt your credentials in your code, they must be decrypted to plaintext before passing to mysqlpp::Connection.

namespace mysql_helpers
{

    auto test_print(const std::vector<mysqlpp::Row>& result)
    {
        std::ostringstream out;
        for (const auto& row : result)
        {
            out << "row: ";

            for (size_t i = 0; i < row.size(); ++i)
            {
                
                out << row[(int)i] << (i + 1 < row.size() ? ", " : "");
            }
            out << "\n";
        }
        return out;
    }

    crow::json::wvalue to_crow_json(const std::vector<mysqlpp::Row>& rows,
                                    const std::vector<std::string>& names)
    {
        crow::json::wvalue::list arr;
        for (const auto& row : rows)
        {
            crow::json::wvalue obj;
            for (size_t index = 0; index < names.size(); ++index)
            {
                int pos = (int)index;
                if (row == NULL)
                {
                    obj[names[pos]] = nullptr;
                }
                else
                {
                    obj[names[pos]] = std::string(row[pos].c_str());
                }
            }
            arr.push_back(std::move(obj));
        }
        return crow::json::wvalue(std::move(arr));
    }
    
    std::string create_key_string(const crow::json::rvalue& data)
    {
        std::string result = "(";
        auto it = data.begin();
        auto end = data.end();
        while (it != end)
        {
            result += "`" + std::string(it->key()) + "`";
            ++it;
            if (it != end)
            {
                result += ", ";
            }
        }
        result += ")";
        return result;
    }

    std::string create_value_row_string(const crow::json::rvalue& row)
    {
        std::string result = "(";
        auto it = row.begin();
        auto end = row.end();
        while (it != end)
        {
            result += "'" + std::string((*it).s()) + "'";
            ++it;
            if (it != end)
            {
                result += ", ";
            }
        }
        result += ")";
        return result;
    }

    std::string create_values_string(const crow::json::rvalue& data)
    {
        std::string result;
        auto it = data.begin();
        auto end = data.end();
        while (it != end)
        {
            result += create_value_row_string(*it);
            ++it; 
            if (it != end)
            {
                result += ", ";
            }
        }
        // msg += ";";
        return result;
    }


    std::string crow_json_converter(const crow::json::rvalue& val)
    {
        std::string result;
        switch (val.t())
        {
            case crow::json::type::String:
                result += "'" + std::string(val.s()) + "'";
                break;
            case crow::json::type::Number:
                result += std::to_string(val.i());
                break;
            case crow::json::type::True:
                result += "TRUE";
                break;
            case crow::json::type::False:
                result += "FALSE";
                break;
            default:
                result += "NULL";
                break;
        }
        return result;
    }
    
    std::string create_update_fields_string(const crow::json::rvalue& data)
    {
        std::string result;
        auto it = data.begin();
        auto end = data.end();
        while (it != end)
        {
            const auto& val = *it;
            result += "`" + std::string(it->key()) + "`=";
            result += crow_json_converter(val);
            ++it;
            if (it != end)
            {
                result += ", ";
            }
        }
        return result;
    }
};
