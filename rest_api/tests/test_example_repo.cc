#include <gtest/gtest.h>
#include "example_repository.hpp"
#include "court_doc_repository.hpp"

//ExampleUsersRepositoryImpl 
TEST(FakeRepo, CreateAssignsIdAndReturnsSuccess)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;
    ExampleUser u = FakeExampleUsersRepository::make_user("Jean", "jean.jean@email.com");

    EXPECT_EQ(repo.create(u), EXIT_SUCCESS);
    ExampleUser created = repo.get_mapped_entry();
    EXPECT_GT(int(created.id), 0);
    EXPECT_EQ(std::string(created.name), "Jean");
    EXPECT_EQ(std::string(created.email), "jean.jean@email.com");
}

TEST(FakeRepo, DuplicateEmailFails)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;
    EXPECT_EQ(repo.create(FakeExampleUsersRepository::make_user("Jean", "jean.jean@email.com")), EXIT_SUCCESS);
    EXPECT_EQ(repo.create(FakeExampleUsersRepository::make_user("Antoine", "jean.jean@email.com")), EXIT_FAILURE);
    EXPECT_STREQ(repo.error(), "duplicate email");
}

TEST(FakeRepo, GetByIdSuccessAndFailure)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;
    repo.create(FakeExampleUsersRepository::make_user("Jean", "jean.jean@email.com"));
    int id = repo.get_mapped_entry().id;

    EXPECT_EQ(repo.get_by_id(id), EXIT_SUCCESS);
    EXPECT_EQ(std::string(repo.get_mapped_entry().name), "Jean");

    EXPECT_EQ(repo.get_by_id(9999), EXIT_FAILURE);
    EXPECT_STREQ(repo.error(), "not found");
}

TEST(FakeRepo, UpdateHappyPath)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;
    repo.create(FakeExampleUsersRepository::make_user("Antoine", "jean.jean@email.com"));
    ExampleUser u = repo.get_mapped_entry();
    u.name = "Antoine";
    EXPECT_EQ(repo.update(u), EXIT_SUCCESS);

    EXPECT_EQ(repo.get_by_id(u.id), EXIT_SUCCESS);
    EXPECT_EQ(std::string(repo.get_mapped_entry().name), "Antoine");
}

TEST(FakeRepo, UpdateNotFound)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;
    ExampleUser ghost = FakeExampleUsersRepository::make_user("Jean", "jean.jean@email.com", 42);
    EXPECT_EQ(repo.update(ghost), EXIT_FAILURE);
    EXPECT_STREQ(repo.error(), "not found");
}

TEST(FakeRepo, RemoveWorksAndThenGetFails)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;
    repo.create(FakeExampleUsersRepository::make_user("Jean", "jean.jean@email.com"));
    int id = repo.get_mapped_entry().id;

    EXPECT_EQ(repo.remove(id), EXIT_SUCCESS);
    EXPECT_EQ(repo.get_by_id(id), EXIT_FAILURE);
}

TEST(FakeRepo, ListAllOrdersDescAndPaginates)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;
    repo.create(FakeExampleUsersRepository::make_user("Jean", "jean.jean@email.com"));
    int id1 = repo.get_mapped_entry().id;
    repo.create(FakeExampleUsersRepository::make_user("Antoine", "Antoine.Antoine@email.com"));
    int id2 = repo.get_mapped_entry().id;
    repo.create(FakeExampleUsersRepository::make_user("Charlie", "charlies.charlie@email.com"));
    int id3 = repo.get_mapped_entry().id;

    // limit 2, offset 0 -> highest ids first
    EXPECT_EQ(repo.list_all(2, 0), EXIT_SUCCESS);
    auto v = repo.get_mapped_entry_vector();
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(int(v[0].id), id3);
    EXPECT_EQ(int(v[1].id), id2);

    // next page
    EXPECT_EQ(repo.list_all(2, 2), EXIT_SUCCESS);
    v = repo.get_mapped_entry_vector();
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(int(v[0].id), id1);
}

// -------
// Non regression testing
TEST(FakeRepo, GetById_NotFound_ClearsStaleData_AndSetsError)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;

    // Seed one user so we have something "stale" to worry about.
    ASSERT_EQ(repo.create(FakeExampleUsersRepository::make_user("Charlie", "charlie.charlie@email.com")), EXIT_SUCCESS);
    ExampleUser seeded = repo.get_mapped_entry();
    ASSERT_GT(int(seeded.id), 0);

    // Read a non-existent id => should fail, set error, and not leave stale data.
    EXPECT_EQ(repo.get_by_id(999999), EXIT_FAILURE);
    EXPECT_STREQ(repo.error(), "not found");

    ExampleUser after = repo.get_mapped_entry();
    // We expect the repo to clear mapped_entry_ on failure:
    EXPECT_EQ(int(after.id), 0);
    EXPECT_TRUE(std::string(after.name).empty());
    EXPECT_TRUE(std::string(after.email).empty());
}

TEST(FakeRepo, Update_NotFound_DoesNotChangeAnything)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;

    ExampleUser ghost = FakeExampleUsersRepository::make_user("Charlie", "charlie.charlie@email.com", 42);
    EXPECT_EQ(repo.update(ghost), EXIT_FAILURE);
    EXPECT_STREQ(repo.error(), "not found");

    // List should be empty still
    EXPECT_EQ(repo.list_all(10, 0), EXIT_SUCCESS);
    EXPECT_TRUE(repo.get_mapped_entry_vector().empty());
}

TEST(FakeRepo, Remove_NotFound_ReturnsFailure)
{
    ExampleUsersRepositoryImpl concrete;
    IExampleUsersRepository& repo = concrete;

    EXPECT_EQ(repo.remove(12345), EXIT_FAILURE);
    EXPECT_STREQ(repo.error(), "not found");
}


namespace
{
CourtDocument make_court_document(int id,
                                  const std::string& filename,
                                  const std::string& full_text,
                                  const std::string& source = "doj",
                                  int page_count = 1)
{
    CourtDocument doc;
    doc.document_id = id;
    doc.filename = filename;
    doc.source = source;
    doc.page_count = page_count;
    doc.full_text = full_text;
    doc.created_at = mysqlpp::DateTime();
    return doc;
}

std::vector<int> document_ids(const std::vector<CourtDocument>& docs)
{
    std::vector<int> ids;
    for (const auto& doc : docs)
    {
        ids.push_back(doc.document_id);
    }
    return ids;
}

std::vector<int> search_result_ids(const std::vector<cdsr_t>& hits)
{
    std::vector<int> ids;
    for (const auto& hit : hits)
    {
        ids.push_back(hit.document_id);
    }
    return ids;
}
}

TEST(FakeCourtDocumentsRepo, ListCursorMovesForwardAndBackwardFromOffsetPage)
{
    FakeCourtDocumentsRepository repo;
    for (int id = 1; id <= 6; ++id)
    {
        repo.documents_by_id_[id] = make_court_document(id, "doc-" + std::to_string(id), "alpha");
    }

    ASSERT_EQ(repo.list_all(2, 2), EXIT_SUCCESS);
    EXPECT_EQ(document_ids(repo.get_mapped_entry_vector()), (std::vector<int>{4, 3}));

    pagination_cursor_t next_cursor;
    next_cursor.direction = pagination_direction_t::next;
    next_cursor.document_id = 3;
    ASSERT_EQ(repo.list_all(2, 0, {}, 0, 0, next_cursor), EXIT_SUCCESS);
    EXPECT_EQ(document_ids(repo.get_mapped_entry_vector()), (std::vector<int>{2, 1}));

    pagination_cursor_t previous_cursor;
    previous_cursor.direction = pagination_direction_t::previous;
    previous_cursor.document_id = 4;
    ASSERT_EQ(repo.list_all(2, 0, {}, 0, 0, previous_cursor), EXIT_SUCCESS);
    EXPECT_EQ(document_ids(repo.get_mapped_entry_vector()), (std::vector<int>{6, 5}));
}

TEST(FakeCourtDocumentsRepo, FilenameCursorPreservesCompoundOrdering)
{
    FakeCourtDocumentsRepository repo;
    repo.documents_by_id_[6] = make_court_document(6, "report", "alpha");
    repo.documents_by_id_[5] = make_court_document(5, "report-a", "alpha");
    repo.documents_by_id_[4] = make_court_document(4, "report-a", "alpha");
    repo.documents_by_id_[3] = make_court_document(3, "report-z", "alpha");

    ASSERT_EQ(repo.search_by_filename("report", 2, 1), EXIT_SUCCESS);
    EXPECT_EQ(document_ids(repo.get_mapped_entry_vector()), (std::vector<int>{5, 4}));

    pagination_cursor_t next_cursor;
    next_cursor.direction = pagination_direction_t::next;
    next_cursor.document_id = 4;
    next_cursor.filename = "report-a";
    ASSERT_EQ(repo.search_by_filename("report", 2, 0, {}, 0, 0, next_cursor), EXIT_SUCCESS);
    EXPECT_EQ(document_ids(repo.get_mapped_entry_vector()), (std::vector<int>{3}));

    pagination_cursor_t previous_cursor;
    previous_cursor.direction = pagination_direction_t::previous;
    previous_cursor.document_id = 5;
    previous_cursor.filename = "report-a";
    ASSERT_EQ(repo.search_by_filename("report", 2, 0, {}, 0, 0, previous_cursor), EXIT_SUCCESS);
    EXPECT_EQ(document_ids(repo.get_mapped_entry_vector()), (std::vector<int>{6}));
}

TEST(FakeCourtDocumentsRepo, FulltextCursorHandlesTiedScores)
{
    FakeCourtDocumentsRepository repo;
    repo.documents_by_id_[6] = make_court_document(6, "doc-6", "alpha alpha alpha");
    repo.documents_by_id_[5] = make_court_document(5, "doc-5", "alpha alpha alpha");
    repo.documents_by_id_[4] = make_court_document(4, "doc-4", "alpha alpha");
    repo.documents_by_id_[3] = make_court_document(3, "doc-3", "alpha alpha");
    repo.documents_by_id_[2] = make_court_document(2, "doc-2", "alpha");

    ASSERT_EQ(repo.search_fulltext("alpha", 2, 1), EXIT_SUCCESS);
    const auto offset_hits = repo.get_search_results();
    EXPECT_EQ(search_result_ids(offset_hits), (std::vector<int>{5, 4}));

    pagination_cursor_t next_cursor;
    next_cursor.direction = pagination_direction_t::next;
    next_cursor.document_id = offset_hits.back().document_id;
    next_cursor.score = offset_hits.back().score;
    ASSERT_EQ(repo.search_fulltext("alpha", 2, 0, {}, 0, 0, next_cursor), EXIT_SUCCESS);
    EXPECT_EQ(search_result_ids(repo.get_search_results()), (std::vector<int>{3, 2}));

    pagination_cursor_t previous_cursor;
    previous_cursor.direction = pagination_direction_t::previous;
    previous_cursor.document_id = offset_hits.front().document_id;
    previous_cursor.score = offset_hits.front().score;
    ASSERT_EQ(repo.search_fulltext("alpha", 2, 0, {}, 0, 0, previous_cursor), EXIT_SUCCESS);
    EXPECT_EQ(search_result_ids(repo.get_search_results()), (std::vector<int>{6}));
}
