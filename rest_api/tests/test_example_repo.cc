#include <gtest/gtest.h>
#include "example_repository.hpp"

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
