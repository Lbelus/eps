#ifndef DB_COURT_DOC_REPOSITORY_HPP
#define DB_COURT_DOC_REPOSITORY_HPP

#if !REPO_FAKE_ONLY
#include <crow.h>
#endif
#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <mysql_conn_pool.hpp>
/*
 * GENERAL INFORMATION: 
 * 
 * The overall structure of this rest API relies heavily on SSQLS;
 * 
 * You should READ THE DOC regarding Specialized SQL Structures:
 *
 * https://tangentsoft.com/mysqlpp/doc/html/userman/ssqls.html
*/



/*  
 * STEP 1: DEFINE the models with SSQLS (example_users)
 * src: https://tangentsoft.com/mysqlpp/doc/html/userman/ssqls.html
 *
 * Macro general format: sql_create_#(NAME, COMPCOUNT, SETCOUNT, TYPE1, ITEM1, ... TYPE#, ITEM#)
 * # is the number of member variables,
 * NAME is the name of the structure
 *   TYPEx = the type of a member variable
 *   ITEMx = the variable’s name
 *   COMPCOUNT = how many fields are used to auto-generate comparison operators (==, <, etc.)
 *   SETCOUNT = how many leading fields you want to initialize via a generated ctor & set() method
 * 
 * let's implement a sql table with SSQLS:
 *
 * MySQL schema :
 *
 * CREATE TABLE IF NOT EXISTS documents (
 *     document_id INT AUTO_INCREMENT PRIMARY KEY,
 *     filename    VARCHAR(512) NOT NULL UNIQUE,
 *     source      VARCHAR(255) NOT NULL,
 *     page_count  INT NOT NULL,
 *     full_text   LONGTEXT NOT NULL,
 *     created_at  DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
 *     FULLTEXT KEY ft_documents_full_text (full_text),
 *     INDEX idx_documents_source (source)
 * ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
 *
 * CREATE TABLE IF NOT EXISTS pages (
 *     page_id      INT AUTO_INCREMENT PRIMARY KEY,
 *     document_id  INT NOT NULL,
 *     page_number  INT NOT NULL,
 *     page_text    LONGTEXT NOT NULL,
 *     INDEX idx_pages_document (document_id),
 *     UNIQUE KEY uq_pages_document_page (document_id, page_number),
 *     CONSTRAINT fk_pages_document
 *         FOREIGN KEY (document_id)
 *         REFERENCES documents(document_id)
 *         ON DELETE CASCADE
 * ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
 */

sql_create_6(CourtDocument, 1, 5,
    mysqlpp::sql_int,       document_id,
    mysqlpp::sql_varchar,   filename,
    mysqlpp::sql_varchar,   source,
    mysqlpp::sql_int,       page_count,
    mysqlpp::sql_longtext,  full_text,
    mysqlpp::sql_datetime,  created_at
)

sql_create_4(CourtPages, 1, 3,
    mysqlpp::sql_int,       page_id,
    mysqlpp::sql_int,       document_id,
    mysqlpp::sql_int,       page_number,
    mysqlpp::sql_longtext,  page_text
)

sql_create_6(CourtDocumentSearchHit, 1, 5,
    mysqlpp::sql_int,       document_id,
    mysqlpp::sql_varchar,   filename,
    mysqlpp::sql_varchar,   source,
    mysqlpp::sql_int,       page_count,
    mysqlpp::sql_double,    score,
    mysqlpp::sql_datetime,  created_at
)

#ifndef COURT_DOCUMENT_SEARCH_RESULT_STRUCT
#define COURT_DOCUMENT_SEARCH_RESULT_STRUCT
struct court_document_search_result_s
{
    int         document_id {};
    std::string filename;
    std::string source;
    int         page_count {};
    double      score {};
    std::string created_at;
    std::string snippet;
};
typedef struct court_document_search_result_s cdsr_t;
#endif

// Snippet slice offsets are byte positions; nudge them forward off UTF-8
// continuation bytes so a slice never cuts a multi-byte character in half
// and emits invalid UTF-8 into JSON responses.
inline std::size_t utf8_align_forward(const std::string& text, std::size_t pos)
{
    while (pos < text.size() && (static_cast<unsigned char>(text[pos]) & 0xC0) == 0x80)
    {
        ++pos;
    }
    return pos;
}

// OPTIONAL: You can add a convenience ctor
// static ExampleUser make_new_example_user(const std::string& name, const std::string& email)
// {
//     ExampleUser ex_user;
//     ex_user.id = 0;
//     ex_user.name = name;
//     ex_user.email = email;
//     ex_user.created_at = mysqlpp::DateTime(); // will be filled by db on insert
//     return ex_user;
// }

// ==============================================


/* 
* STEP 2: Repository interface ()
*
*/

struct ICourtDocumentsRepository
{
    virtual ~ICourtDocumentsRepository() = default;

    virtual int get_by_id(int id) = 0;
    virtual int list_all(std::size_t limit = 100, std::size_t offset = 0, const std::vector<std::string>& source_filters = {}, std::size_t min_pages = 0, std::size_t max_pages = 0) = 0;
    virtual int get_pages_by_document_id(int document_id) = 0;
    virtual int search_fulltext(const std::string& query, std::size_t limit = 20, std::size_t offset = 0, const std::vector<std::string>& source_filters = {}, std::size_t min_pages = 0, std::size_t max_pages = 0) = 0;
    virtual int search_by_filename(const std::string& query, std::size_t limit = 20, std::size_t offset = 0, const std::vector<std::string>& source_filters = {}, std::size_t min_pages = 0, std::size_t max_pages = 0) = 0;
    virtual const char* error() = 0;
    virtual CourtDocument get_mapped_entry() = 0;
    virtual std::vector<CourtDocument> get_mapped_entry_vector() = 0;
    virtual std::vector<CourtPages> get_mapped_pages_vector() = 0;
    virtual std::vector<cdsr_t> get_search_results() = 0;
};

/* 
* STEP 3: MySQL implementation (uses SSQLS + bound params)
*
*/

#if !REPO_FAKE_ONLY
class MySqlCourtDocumentsRepository : public ICourtDocumentsRepository
{
public:
    explicit MySqlCourtDocumentsRepository(mysqlpp::Connection& conn)
        : conn_(conn)
    {
        std::cout << "CourtDocumentsRepository initialized\n";
    }

    explicit MySqlCourtDocumentsRepository(mysqlpp::ScopedConnection& scoped)
        : conn_(*scoped)
    {
        std::cout << "CourtDocumentsRepository initialized (scoped connection)\n";
    }

    int get_by_id(int id) override
    {
        clear_state();

        mysqlpp::Query query = conn().query(
            "SELECT document_id, filename, source, page_count, full_text, created_at "
            "FROM documents "
            "WHERE document_id = %0"
        );
        query.parse();

        mysqlpp::StoreQueryResult result = query.store(id);
        if (!result)
        {
            error_msg_ = query.error();
            mapped_entry_ = create_empty_document();
            return EXIT_FAILURE;
        }

        if (result.num_rows() == 0)
        {
            error_msg_ = "document not found";
            mapped_entry_ = create_empty_document();
            return EXIT_FAILURE;
        }

        mapped_entry_ = row_to_document(result[0]);
        return EXIT_SUCCESS;
    }

    int list_all(std::size_t limit = 100, std::size_t offset = 0, const std::vector<std::string>& source_filters = {}, std::size_t min_pages = 0, std::size_t max_pages = 0) override
    {
        clear_state();

        mysqlpp::StoreQueryResult result;
        if (source_filters.empty())
        {
            mysqlpp::Query query = conn().query(
                "SELECT document_id, filename, source, page_count, created_at "
                "FROM documents "
                "WHERE page_count >= %0 AND (%1 = 0 OR page_count <= %1) "
                "ORDER BY document_id DESC "
                "LIMIT %2 OFFSET %3"
            );
            query.parse();
            result = query.store(
                mysqlpp::sql_int(min_pages),
                mysqlpp::sql_int(max_pages),
                mysqlpp::sql_int(limit),
                mysqlpp::sql_int(offset)
            );
            if (!result)
            {
                error_msg_ = query.error();
                return EXIT_FAILURE;
            }
        }
        else
        {
            mysqlpp::Query query = conn().query(
                "SELECT document_id, filename, source, page_count, created_at "
                "FROM documents "
                "WHERE source IN (%0q, %1q, %2q) AND page_count >= %3 AND (%4 = 0 OR page_count <= %4) "
                "ORDER BY document_id DESC "
                "LIMIT %5 OFFSET %6"
            );
            query.parse();
            result = query.store(
                source_at(source_filters, 0),
                source_at(source_filters, 1),
                source_at(source_filters, 2),
                mysqlpp::sql_int(min_pages),
                mysqlpp::sql_int(max_pages),
                mysqlpp::sql_int(limit),
                mysqlpp::sql_int(offset)
            );
            if (!result)
            {
                error_msg_ = query.error();
                return EXIT_FAILURE;
            }
        }

        mapped_entry_vec_.reserve(result.num_rows());
        for (const auto& row : result)
        {
            mapped_entry_vec_.emplace_back(row_to_document_metadata(row));
        }

        return EXIT_SUCCESS;
    }

    int search_by_filename(const std::string& user_query, std::size_t limit = 20, std::size_t offset = 0, const std::vector<std::string>& source_filters = {}, std::size_t min_pages = 0, std::size_t max_pages = 0) override
    {
        clear_state();

        if (user_query.empty())
        {
            error_msg_ = "empty filename query";
            return EXIT_FAILURE;
        }

        const std::string pattern = "%" + user_query + "%";
        mysqlpp::StoreQueryResult result;
        if (source_filters.empty())
        {
            mysqlpp::Query query = conn().query(
                "SELECT document_id, filename, source, page_count, created_at "
                "FROM documents "
                "WHERE filename LIKE %0q AND page_count >= %1 AND (%2 = 0 OR page_count <= %2) "
                "ORDER BY CASE WHEN filename = %3q THEN 0 ELSE 1 END, filename ASC, document_id DESC "
                "LIMIT %4 OFFSET %5"
            );
            query.parse();
            result = query.store(
                pattern,
                mysqlpp::sql_int(min_pages),
                mysqlpp::sql_int(max_pages),
                user_query,
                mysqlpp::sql_int(limit),
                mysqlpp::sql_int(offset)
            );
            if (!result)
            {
                error_msg_ = query.error();
                return EXIT_FAILURE;
            }
        }
        else
        {
            mysqlpp::Query query = conn().query(
                "SELECT document_id, filename, source, page_count, created_at "
                "FROM documents "
                "WHERE filename LIKE %0q AND source IN (%1q, %2q, %3q) AND page_count >= %4 AND (%5 = 0 OR page_count <= %5) "
                "ORDER BY CASE WHEN filename = %6q THEN 0 ELSE 1 END, filename ASC, document_id DESC "
                "LIMIT %7 OFFSET %8"
            );
            query.parse();
            result = query.store(
                pattern,
                source_at(source_filters, 0),
                source_at(source_filters, 1),
                source_at(source_filters, 2),
                mysqlpp::sql_int(min_pages),
                mysqlpp::sql_int(max_pages),
                user_query,
                mysqlpp::sql_int(limit),
                mysqlpp::sql_int(offset)
            );
            if (!result)
            {
                error_msg_ = query.error();
                return EXIT_FAILURE;
            }
        }

        mapped_entry_vec_.reserve(result.num_rows());
        for (const auto& row : result)
        {
            mapped_entry_vec_.emplace_back(row_to_document_metadata(row));
        }

        return EXIT_SUCCESS;
    }

    int get_pages_by_document_id(int document_id) override
    {
        clear_state();

        mysqlpp::Query query = conn().query(
            "SELECT page_id, document_id, page_number, page_text "
            "FROM pages "
            "WHERE document_id = %0 "
            "ORDER BY page_number ASC"
        );
        query.parse();

        mysqlpp::StoreQueryResult result = query.store(document_id);
        if (!result)
        {
            error_msg_ = query.error();
            return EXIT_FAILURE;
        }

        mapped_pages_vec_.reserve(result.num_rows());
        for (const auto& row : result)
        {
            mapped_pages_vec_.emplace_back(row_to_page(row));
        }

        return EXIT_SUCCESS;
    }

    int search_fulltext(const std::string& user_query, std::size_t limit = 20, std::size_t offset = 0, const std::vector<std::string>& source_filters = {}, std::size_t min_pages = 0, std::size_t max_pages = 0) override
    {
        clear_state();

        if (user_query.empty())
        {
            error_msg_ = "empty search query";
            return EXIT_FAILURE;
        }

        // Snippets need only the text around the first match, so fetch a
        // bounded excerpt instead of the whole LONGTEXT per hit. LOCATE is
        // case-insensitive under utf8mb4_unicode_ci. If the anchor term is
        // absent LOCATE yields 0 and the window falls back to the document
        // start, matching build_snippet's own fallback.
        const std::vector<std::string> anchor_terms = split_terms(user_query);
        const std::string anchor = anchor_terms.empty() ? user_query : anchor_terms.front();

        mysqlpp::StoreQueryResult result;
        if (source_filters.empty())
        {
            mysqlpp::Query query = conn().query(
                "SELECT "
                "  document_id, "
                "  filename, "
                "  source, "
                "  page_count, "
                "  MATCH(full_text) AGAINST (%0q IN NATURAL LANGUAGE MODE) AS score, "
                "  created_at, "
                "  SUBSTRING(full_text, "
                "            GREATEST(CAST(LOCATE(%1q, full_text) AS SIGNED) - 4000, 1), "
                "            8000) AS excerpt "
                "FROM documents "
                "WHERE MATCH(full_text) AGAINST (%2q IN NATURAL LANGUAGE MODE) AND page_count >= %3 AND (%4 = 0 OR page_count <= %4) "
                "ORDER BY score DESC, document_id DESC "
                "LIMIT %5 OFFSET %6"
            );
            query.parse();
            result = query.store(
                user_query,
                anchor,
                user_query,
                mysqlpp::sql_int(min_pages),
                mysqlpp::sql_int(max_pages),
                mysqlpp::sql_int(limit),
                mysqlpp::sql_int(offset)
            );
            if (!result)
            {
                error_msg_ = query.error();
                return EXIT_FAILURE;
            }
        }
        else
        {
            mysqlpp::Query query = conn().query(
                "SELECT "
                "  document_id, "
                "  filename, "
                "  source, "
                "  page_count, "
                "  MATCH(full_text) AGAINST (%0q IN NATURAL LANGUAGE MODE) AS score, "
                "  created_at, "
                "  SUBSTRING(full_text, "
                "            GREATEST(CAST(LOCATE(%1q, full_text) AS SIGNED) - 4000, 1), "
                "            8000) AS excerpt "
                "FROM documents "
                "WHERE MATCH(full_text) AGAINST (%2q IN NATURAL LANGUAGE MODE) AND source IN (%3q, %4q, %5q) AND page_count >= %6 AND (%7 = 0 OR page_count <= %7) "
                "ORDER BY score DESC, document_id DESC "
                "LIMIT %8 OFFSET %9"
            );
            query.parse();
            result = query.store(
                user_query,
                anchor,
                user_query,
                source_at(source_filters, 0),
                source_at(source_filters, 1),
                source_at(source_filters, 2),
                mysqlpp::sql_int(min_pages),
                mysqlpp::sql_int(max_pages),
                mysqlpp::sql_int(limit),
                mysqlpp::sql_int(offset)
            );
            if (!result)
            {
                error_msg_ = query.error();
                return EXIT_FAILURE;
            }
        }

        search_results_.reserve(result.num_rows());
        for (const auto& row : result)
        {
            cdsr_t hit;
            hit.document_id = int(row[0]);
            hit.filename    = safe_string(row[1]);
            hit.source      = safe_string(row[2]);
            hit.page_count  = int(row[3]);
            hit.score       = double(row[4]);
            hit.created_at  = safe_string(row[5]);

            const std::string excerpt = safe_string(row[6]);
            hit.snippet = build_snippet(excerpt, user_query, 120);

            search_results_.push_back(std::move(hit));
        }

        return EXIT_SUCCESS;
    }

    const char* error() override
    {
        return error_msg_.c_str();
    }

    CourtDocument get_mapped_entry() override
    {
        return mapped_entry_;
    }

    std::vector<CourtDocument> get_mapped_entry_vector() override
    {
        return mapped_entry_vec_;
    }

    std::vector<CourtPages> get_mapped_pages_vector() override
    {
        return mapped_pages_vec_;
    }

    std::vector<cdsr_t> get_search_results() override
    {
        return search_results_;
    }

#if !REPO_FAKE_ONLY
    static crow::json::wvalue to_crow_json(const CourtDocument& doc)
    {
        crow::json::wvalue obj;
        obj["document_id"] = doc.document_id;
        obj["filename"]    = std::string(doc.filename);
        obj["source"]      = std::string(doc.source);
        obj["page_count"]  = doc.page_count;
        obj["full_text"]   = std::string(doc.full_text);
        obj["created_at"]  = doc.created_at.str();
        return obj;
    }

    // Metadata-only projection: full_text is served by GET /courtdocuments/<id>
    // and per page via /courtdocuments/<id>/pages, never in list responses.
    static crow::json::wvalue to_crow_json_metadata(const CourtDocument& doc)
    {
        crow::json::wvalue obj;
        obj["document_id"] = doc.document_id;
        obj["filename"]    = std::string(doc.filename);
        obj["source"]      = std::string(doc.source);
        obj["page_count"]  = doc.page_count;
        obj["created_at"]  = doc.created_at.str();
        return obj;
    }

    static crow::json::wvalue to_crow_json(const std::vector<CourtDocument>& docs)
    {
        crow::json::wvalue::list arr;
        arr.reserve(docs.size());
        for (const auto& doc : docs)
        {
            arr.push_back(to_crow_json_metadata(doc));
        }
        return crow::json::wvalue(arr);
    }

    static crow::json::wvalue to_crow_json(const CourtPages& page)
    {
        crow::json::wvalue obj;
        obj["page_id"]      = page.page_id;
        obj["document_id"]  = page.document_id;
        obj["page_number"]  = page.page_number;
        obj["page_text"]    = std::string(page.page_text);
        return obj;
    }

    static crow::json::wvalue to_crow_json(const std::vector<CourtPages>& pages)
    {
        crow::json::wvalue::list arr;
        arr.reserve(pages.size());
        for (const auto& page : pages)
        {
            arr.push_back(to_crow_json(page));
        }
        return crow::json::wvalue(arr);
    }

    static crow::json::wvalue to_crow_json(const cdsr_t& hit)
    {
        crow::json::wvalue obj;
        obj["document_id"] = hit.document_id;
        obj["filename"]    = hit.filename;
        obj["source"]      = hit.source;
        obj["page_count"]  = hit.page_count;
        obj["score"]       = hit.score;
        obj["created_at"]  = hit.created_at;
        obj["snippet"]     = hit.snippet;
        return obj;
    }

    static crow::json::wvalue to_crow_json(const std::vector<cdsr_t>& hits)
    {
        crow::json::wvalue::list arr;
        arr.reserve(hits.size());
        for (const auto& hit : hits)
        {
            arr.push_back(to_crow_json(hit));
        }
        return crow::json::wvalue(arr);
    }
#endif

private:
    std::reference_wrapper<mysqlpp::Connection> conn_;
    std::string error_msg_;
    CourtDocument mapped_entry_ = create_empty_document();
    std::vector<CourtDocument> mapped_entry_vec_;
    std::vector<CourtPages> mapped_pages_vec_;
    std::vector<cdsr_t> search_results_;

    mysqlpp::Connection& conn()
    {
        return conn_.get();
    }

    void clear_state()
    {
        error_msg_.clear();
        mapped_entry_ = create_empty_document();
        mapped_entry_vec_.clear();
        mapped_pages_vec_.clear();
        search_results_.clear();
    }

    static CourtDocument create_empty_document()
    {
        CourtDocument doc;
        doc.document_id = 0;
        doc.filename = "";
        doc.source = "";
        doc.page_count = 0;
        doc.full_text = "";
        doc.created_at = mysqlpp::DateTime();
        return doc;
    }

    static CourtPages create_empty_page()
    {
        CourtPages page;
        page.page_id = 0;
        page.document_id = 0;
        page.page_number = 0;
        page.page_text = "";
        return page;
    }

    static std::string source_at(const std::vector<std::string>& source_filters, std::size_t index)
    {
        return index < source_filters.size() ? source_filters[index] : "__missing_source__";
    }

    static std::string safe_string(const mysqlpp::String& s)
    {
        if (s.is_null())
            return {};
        return std::string(s.c_str());
    }
    static CourtDocument row_to_document(const mysqlpp::Row& row)
    {
        CourtDocument doc = create_empty_document();
        doc.document_id = int(row[0]);
        doc.filename    = safe_string(row[1]);
        doc.source      = safe_string(row[2]);
        doc.page_count  = int(row[3]);
        doc.full_text   = safe_string(row[4]);
        doc.created_at  = mysqlpp::DateTime(safe_string(row[5]));
        return doc;
    }

    // For list queries that skip the LONGTEXT column; full_text stays empty.
    static CourtDocument row_to_document_metadata(const mysqlpp::Row& row)
    {
        CourtDocument doc = create_empty_document();
        doc.document_id = int(row[0]);
        doc.filename    = safe_string(row[1]);
        doc.source      = safe_string(row[2]);
        doc.page_count  = int(row[3]);
        doc.created_at  = mysqlpp::DateTime(safe_string(row[4]));
        return doc;
    }

    static CourtPages row_to_page(const mysqlpp::Row& row)
    {
        CourtPages page = create_empty_page();
        page.page_id      = int(row[0]);
        page.document_id  = int(row[1]);
        page.page_number  = int(row[2]);
        page.page_text    = safe_string(row[3]);
        return page;
    }

    static std::string to_lower_copy(const std::string& input)
    {
        std::string out = input;
        std::transform(out.begin(), out.end(), out.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }

    static std::vector<std::string> split_terms(const std::string& query)
    {
        std::vector<std::string> terms;
        std::istringstream iss(query);
        std::string token;

        while (iss >> token)
        {
            std::string clean;
            clean.reserve(token.size());

            for (unsigned char c : token)
            {
                if (std::isalnum(c))
                {
                    clean.push_back(static_cast<char>(std::tolower(c)));
                }
            }

            if (clean.size() >= 2)
            {
                terms.push_back(clean);
            }
        }

        std::sort(terms.begin(), terms.end());
        terms.erase(std::unique(terms.begin(), terms.end()), terms.end());
        return terms;
    }

    static std::string highlight_terms(std::string snippet, const std::vector<std::string>& terms)
    {
        if (snippet.empty() || terms.empty())
        {
            return snippet;
        }

        std::string lower_snippet = to_lower_copy(snippet);

        for (const auto& term : terms)
        {
            std::size_t pos = 0;
            while ((pos = lower_snippet.find(term, pos)) != std::string::npos)
            {
                snippet.insert(pos, ">>>");
                lower_snippet.insert(pos, ">>>");
                pos += 3 + term.size();

                snippet.insert(pos, "<<<");
                lower_snippet.insert(pos, "<<<");
                pos += 3;
            }
        }

        return snippet;
    }

    static std::string build_snippet(const std::string& text,
                                     const std::string& query,
                                     std::size_t radius = 120)
    {
        if (text.empty())
        {
            return "";
        }

        const std::vector<std::string> terms = split_terms(query);
        const std::string lower_text = to_lower_copy(text);

        std::size_t best_pos = std::string::npos;
        std::size_t best_len = 0;

        for (const auto& term : terms)
        {
            std::size_t pos = lower_text.find(term);
            if (pos != std::string::npos)
            {
                best_pos = pos;
                best_len = term.size();
                break;
            }
        }

        if (best_pos == std::string::npos)
        {
            std::string fallback = text.substr(0, utf8_align_forward(text, std::min(text.size(), radius * 2)));
            if (text.size() > fallback.size())
            {
                fallback += "...";
            }
            return fallback;
        }

        const std::size_t start = utf8_align_forward(text, (best_pos > radius) ? best_pos - radius : 0);
        const std::size_t end = utf8_align_forward(text, std::min(text.size(), best_pos + best_len + radius));
        std::string snippet = text.substr(start, end - start);

        if (start > 0)
        {
            snippet = "..." + snippet;
        }
        if (end < text.size())
        {
            snippet += "...";
        }

        return highlight_terms(snippet, terms);
    }
};
#endif // !REPO_FAKE_ONLY
/* 
* STEP 4: Use the virtual interface to create a test class
*
*/
#if REPO_ENABLE_FAKE

/*
 * STEP 4: Fake implementation
 */
class FakeCourtDocumentsRepository : public ICourtDocumentsRepository
{
public:
    std::unordered_map<int, CourtDocument> documents_by_id_;
    std::unordered_map<int, std::vector<CourtPages>> pages_by_document_id_;
    CourtDocument mapped_entry_ = create_empty_document();
    std::vector<CourtDocument> mapped_vec_;
    std::vector<CourtPages> mapped_pages_vec_;
    std::vector<CourtDocumentSearchResult> search_results_;
    std::string last_error_;

    int get_by_id(int id) override
    {
        clear_state();

        auto it = documents_by_id_.find(id);
        if (it == documents_by_id_.end())
        {
            last_error_ = "document not found";
            mapped_entry_ = create_empty_document();
            return EXIT_FAILURE;
        }

        mapped_entry_ = it->second;
        return EXIT_SUCCESS;
    }

    int list_all(std::size_t limit = 100, std::size_t offset = 0, const std::vector<std::string>& source_filters = {}, std::size_t min_pages = 0, std::size_t max_pages = 0) override
    {
        clear_state();

        std::vector<int> ids;
        ids.reserve(documents_by_id_.size());

        for (const auto& kv : documents_by_id_)
        {
            if (!source_matches(source_filters, std::string(kv.second.source)) || !page_count_matches(kv.second.page_count, min_pages, max_pages))
            {
                continue;
            }
            ids.push_back(kv.first);
        }

        std::sort(ids.begin(), ids.end(), std::greater<int>());

        std::size_t seen = 0;
        for (int id : ids)
        {
            if (seen++ < offset)
            {
                continue;
            }
            if (mapped_vec_.size() >= limit)
            {
                break;
            }
            mapped_vec_.push_back(documents_by_id_[id]);
        }

        return EXIT_SUCCESS;
    }

    int search_by_filename(const std::string& query,
                           std::size_t limit = 20,
                           std::size_t offset = 0,
                           const std::vector<std::string>& source_filters = {}, std::size_t min_pages = 0, std::size_t max_pages = 0) override
    {
        clear_state();

        if (query.empty())
        {
            last_error_ = "empty filename query";
            return EXIT_FAILURE;
        }

        const std::string needle = to_lower_copy(query);
        std::vector<CourtDocument> hits;

        for (const auto& kv : documents_by_id_)
        {
            const CourtDocument& doc = kv.second;
            if (!source_matches(source_filters, std::string(doc.source)) || !page_count_matches(doc.page_count, min_pages, max_pages))
            {
                continue;
            }
            const std::string filename = std::string(doc.filename);
            if (to_lower_copy(filename).find(needle) != std::string::npos)
            {
                hits.push_back(doc);
            }
        }

        std::sort(hits.begin(), hits.end(),
            [&needle](const CourtDocument& a, const CourtDocument& b)
            {
                const std::string a_filename = to_lower_copy(std::string(a.filename));
                const std::string b_filename = to_lower_copy(std::string(b.filename));
                const bool a_exact = a_filename == needle;
                const bool b_exact = b_filename == needle;
                if (a_exact != b_exact)
                {
                    return a_exact;
                }
                if (a_filename != b_filename)
                {
                    return a_filename < b_filename;
                }
                return a.document_id > b.document_id;
            });

        for (std::size_t i = offset; i < hits.size() && mapped_vec_.size() < limit; ++i)
        {
            mapped_vec_.push_back(hits[i]);
        }

        return EXIT_SUCCESS;
    }

    int get_pages_by_document_id(int document_id) override
    {
        clear_state();

        auto it = pages_by_document_id_.find(document_id);
        if (it == pages_by_document_id_.end())
        {
            return EXIT_SUCCESS;
        }

        mapped_pages_vec_ = it->second;
        std::sort(mapped_pages_vec_.begin(), mapped_pages_vec_.end(),
            [](const CourtPages& a, const CourtPages& b)
            {
                return a.page_number < b.page_number;
            });

        return EXIT_SUCCESS;
    }

    int search_fulltext(const std::string& query,
                        std::size_t limit = 20,
                        std::size_t offset = 0,
                        const std::vector<std::string>& source_filters = {}, std::size_t min_pages = 0, std::size_t max_pages = 0) override
    {
        clear_state();

        if (query.empty())
        {
            last_error_ = "empty search query";
            return EXIT_FAILURE;
        }

        struct TempHit
        {
            CourtDocumentSearchResult result;
        };

        std::vector<TempHit> hits;
        const std::vector<std::string> terms = split_terms(query);

        for (const auto& kv : documents_by_id_)
        {
            const CourtDocument& doc = kv.second;
            if (!source_matches(source_filters, std::string(doc.source)) || !page_count_matches(doc.page_count, min_pages, max_pages))
            {
                continue;
            }
            const std::string full_text = std::string(doc.full_text);
            const std::string lower_text = to_lower_copy(full_text);

            double score = 0.0;
            bool matched = false;

            for (const auto& term : terms)
            {
                std::size_t pos = 0;
                while ((pos = lower_text.find(term, pos)) != std::string::npos)
                {
                    matched = true;
                    score += 1.0;
                    pos += term.size();
                }
            }

            if (!matched)
            {
                continue;
            }

            CourtDocumentSearchResult hit;
            hit.document_id = doc.document_id;
            hit.filename    = std::string(doc.filename);
            hit.source      = std::string(doc.source);
            hit.page_count  = doc.page_count;
            hit.score       = score;
            hit.created_at  = doc.created_at.str();
            hit.snippet     = build_snippet(full_text, query, 120);

            hits.push_back({hit});
        }

        std::sort(hits.begin(), hits.end(),
            [](const TempHit& a, const TempHit& b)
            {
                if (a.result.score != b.result.score)
                {
                    return a.result.score > b.result.score;
                }
                return a.result.document_id > b.result.document_id;
            });

        for (std::size_t i = offset; i < hits.size() && search_results_.size() < limit; ++i)
        {
            search_results_.push_back(hits[i].result);
        }

        return EXIT_SUCCESS;
    }

    const char* error() override
    {
        return last_error_.c_str();
    }

    CourtDocument get_mapped_entry() override
    {
        return mapped_entry_;
    }

    std::vector<CourtDocument> get_mapped_entry_vector() override
    {
        return mapped_vec_;
    }

    std::vector<CourtPages> get_mapped_pages_vector() override
    {
        return mapped_pages_vec_;
    }

    std::vector<CourtDocumentSearchResult> get_search_results() override
    {
        return search_results_;
    }

private:
    void clear_state()
    {
        last_error_.clear();
        mapped_entry_ = create_empty_document();
        mapped_vec_.clear();
        mapped_pages_vec_.clear();
        search_results_.clear();
    }

    static CourtDocument create_empty_document()
    {
        CourtDocument doc;
        doc.document_id = 0;
        doc.filename = "";
        doc.source = "";
        doc.page_count = 0;
        doc.full_text = "";
        doc.created_at = mysqlpp::DateTime();
        return doc;
    }

    static bool source_matches(const std::vector<std::string>& source_filters, const std::string& source)
    {
        return source_filters.empty() ||
            std::find(source_filters.begin(), source_filters.end(), source) != source_filters.end();
    }

    static bool page_count_matches(int page_count, std::size_t min_pages, std::size_t max_pages)
    {
        return page_count >= static_cast<int>(min_pages) &&
            (max_pages == 0 || page_count <= static_cast<int>(max_pages));
    }

    static std::string to_lower_copy(const std::string& input)
    {
        std::string out = input;
        std::transform(out.begin(), out.end(), out.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return out;
    }

    static std::vector<std::string> split_terms(const std::string& query)
    {
        std::vector<std::string> terms;
        std::istringstream iss(query);
        std::string token;

        while (iss >> token)
        {
            std::string clean;
            clean.reserve(token.size());

            for (unsigned char c : token)
            {
                if (std::isalnum(c))
                {
                    clean.push_back(static_cast<char>(std::tolower(c)));
                }
            }

            if (clean.size() >= 2)
            {
                terms.push_back(clean);
            }
        }

        std::sort(terms.begin(), terms.end());
        terms.erase(std::unique(terms.begin(), terms.end()), terms.end());
        return terms;
    }

    static std::string highlight_terms(std::string snippet, const std::vector<std::string>& terms)
    {
        if (snippet.empty() || terms.empty())
        {
            return snippet;
        }

        std::string lower_snippet = to_lower_copy(snippet);

        for (const auto& term : terms)
        {
            std::size_t pos = 0;
            while ((pos = lower_snippet.find(term, pos)) != std::string::npos)
            {
                snippet.insert(pos, ">>>");
                lower_snippet.insert(pos, ">>>");
                pos += 3 + term.size();

                snippet.insert(pos, "<<<");
                lower_snippet.insert(pos, "<<<");
                pos += 3;
            }
        }

        return snippet;
    }

    static std::string build_snippet(const std::string& text, const std::string& query, std::size_t radius = 120)
    {
        if (text.empty())
        {
            return "";
        }

        const std::vector<std::string> terms = split_terms(query);
        const std::string lower_text = to_lower_copy(text);

        std::size_t best_pos = std::string::npos;
        std::size_t best_len = 0;

        for (const auto& term : terms)
        {
            std::size_t pos = lower_text.find(term);
            if (pos != std::string::npos)
            {
                best_pos = pos;
                best_len = term.size();
                break;
            }
        }

        if (best_pos == std::string::npos)
        {
            std::string fallback = text.substr(0, utf8_align_forward(text, std::min(text.size(), radius * 2)));
            if (text.size() > fallback.size())
            {
                fallback += "...";
            }
            return fallback;
        }

        const std::size_t start = utf8_align_forward(text, (best_pos > radius) ? best_pos - radius : 0);
        const std::size_t end = utf8_align_forward(text, std::min(text.size(), best_pos + best_len + radius));
        std::string snippet = text.substr(start, end - start);

        if (start > 0)
        {
            snippet = "..." + snippet;
        }
        if (end < text.size())
        {
            snippet += "...";
        }

        return highlight_terms(snippet, terms);
    }
};

#endif // REPO_ENABLE_FAKE
/* 
* STEP 5: Create some route (crow app + pool_ptr)
*
*/
#if REPO_ENABLE_FAKE
using CourtDocumentsRepositoryImpl = FakeCourtDocumentsRepository;
#else
using CourtDocumentsRepositoryImpl = MySqlCourtDocumentsRepository;
#endif

#if !REPO_FAKE_ONLY

/*
 * STEP 6: Routes
 */

// std::stoul throws on garbage input; malformed or oversized values fall
// back to the default / cap so a bad query string can't 500 the handler
// or request an unbounded result set.
inline std::size_t parse_size_param(const char* raw, std::size_t fallback, std::size_t max_value)
{
    if (!raw || *raw == '\0')
    {
        return fallback;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(raw, &end, 10);
    if (end == raw || *end != '\0')
    {
        return fallback;
    }

    return std::min<std::size_t>(parsed, max_value);
}
inline bool parse_page_count_param(const char* raw, std::size_t& value, std::size_t fallback, std::size_t max_value)
{
    value = fallback;
    if (!raw || *raw == '\0')
    {
        return true;
    }
    if (raw[0] == '-')
    {
        return false;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(raw, &end, 10);
    if (end == raw || *end != '\0')
    {
        return false;
    }

    value = std::min<std::size_t>(parsed, max_value);
    return true;
}


inline bool parse_source_param(const char* raw, std::vector<std::string>& source_filters)
{
    source_filters.clear();
    if (!raw || *raw == '\0')
    {
        return true;
    }

    std::stringstream tokens(raw);
    std::string token;
    while (std::getline(tokens, token, ','))
    {
        token.erase(std::remove_if(token.begin(), token.end(),
            [](unsigned char c) { return std::isspace(c); }), token.end());

        if (token.empty())
        {
            return false;
        }
        if (token == "all")
        {
            source_filters.clear();
            return true;
        }
        if (token != "doj" && token != "cl" && token != "dc")
        {
            return false;
        }
        if (std::find(source_filters.begin(), source_filters.end(), token) == source_filters.end())
        {
            source_filters.push_back(token);
        }
    }

    if (source_filters.size() == 3)
    {
        source_filters.clear();
    }

    return true;
}

template <typename... Middlewares>
void mysqlCourtDocuments_routes(crow::Crow<Middlewares...>& app, SimpleConnectionPool& pool_ptr)
{
    // GET /courtdocuments/<id>
    CROW_ROUTE(app, "/courtdocuments/<int>").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](int id)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        CourtDocumentsRepositoryImpl repo(sc);

        int result = repo.get_by_id(id);
        if (result != EXIT_SUCCESS)
        {
            return crow::response(404, repo.error());
        }

        CourtDocument doc = repo.get_mapped_entry();
        return crow::response(200, MySqlCourtDocumentsRepository::to_crow_json(doc));
    });

    // GET /courtdocuments?limit=&offset=
    CROW_ROUTE(app, "/courtdocuments").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](const crow::request& req)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        CourtDocumentsRepositoryImpl repo(sc);

        std::size_t limit  = parse_size_param(req.url_params.get("limit"), 100, 100);
        std::size_t offset = parse_size_param(req.url_params.get("offset"), 0, 1000000);
        std::vector<std::string> source_filters;
        if (!parse_source_param(req.url_params.get("source"), source_filters))
        {
            return crow::response(400, "Invalid source parameter");
        }

        std::size_t min_pages = 0;
        std::size_t max_pages = 0;
        if (!parse_page_count_param(req.url_params.get("page_min"), min_pages, 0, 1000000) ||
            !parse_page_count_param(req.url_params.get("page_max"), max_pages, 0, 1000000) ||
            (max_pages != 0 && min_pages > max_pages))
        {
            return crow::response(400, "Invalid page range parameter");
        }

        int result = repo.list_all(limit, offset, source_filters, min_pages, max_pages);
        if (result != EXIT_SUCCESS)
        {
            return crow::response(500, repo.error());
        }

        auto docs = repo.get_mapped_entry_vector();
        return crow::response(200, MySqlCourtDocumentsRepository::to_crow_json(docs));
    });

    // GET /courtdocuments/by-filename?q=...&limit=&offset=
    CROW_ROUTE(app, "/courtdocuments/by-filename").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](const crow::request& req)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        CourtDocumentsRepositoryImpl repo(sc);

        const char* q = req.url_params.get("q");
        if (!q || std::string(q).empty())
        {
            return crow::response(400, "Missing q parameter");
        }

        std::size_t limit  = parse_size_param(req.url_params.get("limit"), 20, 50);
        std::size_t offset = parse_size_param(req.url_params.get("offset"), 0, 10000);
        std::vector<std::string> source_filters;
        if (!parse_source_param(req.url_params.get("source"), source_filters))
        {
            return crow::response(400, "Invalid source parameter");
        }

        std::size_t min_pages = 0;
        std::size_t max_pages = 0;
        if (!parse_page_count_param(req.url_params.get("page_min"), min_pages, 0, 1000000) ||
            !parse_page_count_param(req.url_params.get("page_max"), max_pages, 0, 1000000) ||
            (max_pages != 0 && min_pages > max_pages))
        {
            return crow::response(400, "Invalid page range parameter");
        }

        int result = repo.search_by_filename(q, limit, offset, source_filters, min_pages, max_pages);
        if (result != EXIT_SUCCESS)
        {
            return crow::response(500, repo.error());
        }

        auto docs = repo.get_mapped_entry_vector();
        return crow::response(200, MySqlCourtDocumentsRepository::to_crow_json(docs));
    });

    // GET /courtdocuments/<id>/pages
    CROW_ROUTE(app, "/courtdocuments/<int>/pages").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](int document_id)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        CourtDocumentsRepositoryImpl repo(sc);

        int result = repo.get_pages_by_document_id(document_id);
        if (result != EXIT_SUCCESS)
        {
            return crow::response(500, repo.error());
        }

        auto pages = repo.get_mapped_pages_vector();
        return crow::response(200, MySqlCourtDocumentsRepository::to_crow_json(pages));
    });

    // GET /courtdocuments/search?q=...&limit=&offset=
    CROW_ROUTE(app, "/courtdocuments/search").methods(crow::HTTPMethod::GET)
    ([&pool_ptr](const crow::request& req)
    {
        mysqlpp::ScopedConnection sc(pool_ptr, true);
        CourtDocumentsRepositoryImpl repo(sc);

        const char* q = req.url_params.get("q");
        if (!q || std::string(q).empty())
        {
            return crow::response(400, "Missing q parameter");
        }

        std::size_t limit  = parse_size_param(req.url_params.get("limit"), 20, 50);
        std::size_t offset = parse_size_param(req.url_params.get("offset"), 0, 10000);
        std::vector<std::string> source_filters;
        if (!parse_source_param(req.url_params.get("source"), source_filters))
        {
            return crow::response(400, "Invalid source parameter");
        }

        std::size_t min_pages = 0;
        std::size_t max_pages = 0;
        if (!parse_page_count_param(req.url_params.get("page_min"), min_pages, 0, 1000000) ||
            !parse_page_count_param(req.url_params.get("page_max"), max_pages, 0, 1000000) ||
            (max_pages != 0 && min_pages > max_pages))
        {
            return crow::response(400, "Invalid page range parameter");
        }

        int result = repo.search_fulltext(q, limit, offset, source_filters, min_pages, max_pages);
        if (result != EXIT_SUCCESS)
        {
            return crow::response(500, repo.error());
        }

        auto hits = repo.get_search_results();
        return crow::response(200, MySqlCourtDocumentsRepository::to_crow_json(hits));
    });
}

#endif // !REPO_FAKE_ONLY

#endif // DB_COURT_DOC_REPOSITORY_HPP
