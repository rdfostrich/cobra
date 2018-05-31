#include <gtest/gtest.h>

#include "../../../main/cpp/snapshot/snapshot_manager.h"
#include "../../../main/cpp/snapshot/vector_triple_iterator.h"

#define BASEURI "<http://example.org>"
#define TESTPATH "./"

// The fixture for testing class snapshotManager
class SnapshotManagerTest : public ::testing::Test {
protected:
    SnapshotManager* snapshotManager;
    VectorTripleIterator* it;

    SnapshotManagerTest() : snapshotManager() {}

    virtual void SetUp() {
        snapshotManager = new SnapshotManager(TESTPATH);

        std::vector<TripleString> triples;
        triples.push_back(TripleString("<a>", "<a>", "<a>"));
        triples.push_back(TripleString("<a>", "<a>", "<b>"));
        triples.push_back(TripleString("<a>", "<a>", "<c>"));
        it = new VectorTripleIterator(triples);
    }

    virtual void TearDown() {
        std::map<int, HDT*> patches = snapshotManager->get_snapshots();
        std::map<int, HDT*>::iterator it = patches.begin();
        std::list<int> patchDictsToDelete;
        while(it != patches.end()) {
            int id = it->first;
            std::remove((TESTPATH + SNAPSHOT_FILENAME_BASE(id)).c_str());
            std::remove((TESTPATH + SNAPSHOT_FILENAME_BASE(id) + ".index").c_str());
            patchDictsToDelete.push_back(id);
            it++;
        }
        delete snapshotManager;

        // Delete dictionaries
        std::list<int>::iterator itd;
        for(itd=patchDictsToDelete.begin(); itd!=patchDictsToDelete.end(); ++itd) {
            DictionaryManager::cleanup(TESTPATH, *itd);
        }
    }
};

TEST_F(SnapshotManagerTest, ConstructSnapshot) {
    ASSERT_EQ((HDT*) NULL, snapshotManager->get_snapshot(100));
    snapshotManager->create_snapshot(100, it, BASEURI);
    ASSERT_NE((HDT*) NULL, snapshotManager->get_snapshot(100));
}

TEST_F(SnapshotManagerTest, DetectSnapshotsTrees) {
    ASSERT_EQ(0, snapshotManager->detect_snapshots().size());
    snapshotManager->create_snapshot(100, it, BASEURI);
    ASSERT_EQ(1, snapshotManager->detect_snapshots().size());
    snapshotManager->create_snapshot(200, it, BASEURI);
    ASSERT_EQ(2, snapshotManager->detect_snapshots().size());
}

TEST_F(SnapshotManagerTest, GetSnapshot) {
    HDT* snapshot = snapshotManager->create_snapshot(100, it, BASEURI);
    ASSERT_EQ(snapshot, snapshotManager->get_snapshot(100));
}

TEST_F(SnapshotManagerTest, GetByPatchId) {
    snapshotManager->create_snapshot(0, it, BASEURI);
    snapshotManager->create_snapshot(10, it, BASEURI);
    snapshotManager->create_snapshot(100, it, BASEURI);

    ASSERT_EQ(-1, snapshotManager->get_latest_snapshot(-100));
    ASSERT_EQ(-1, snapshotManager->get_latest_snapshot(-1));
    ASSERT_EQ(0, snapshotManager->get_latest_snapshot(0));
    ASSERT_EQ(0, snapshotManager->get_latest_snapshot(1));
    ASSERT_EQ(0, snapshotManager->get_latest_snapshot(9));
    ASSERT_EQ(10, snapshotManager->get_latest_snapshot(10));
    ASSERT_EQ(10, snapshotManager->get_latest_snapshot(11));
    ASSERT_EQ(10, snapshotManager->get_latest_snapshot(99));
    ASSERT_EQ(100, snapshotManager->get_latest_snapshot(100));
    ASSERT_EQ(100, snapshotManager->get_latest_snapshot(101));
}