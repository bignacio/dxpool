#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <algorithm>
#include <set>

#include "../src/MutexIndexer.h"
#include "../src/ConcurrentIndexer.h"

#include "IndexerTemplateTest.h"

using namespace dxpool;
using namespace std;

TEMPLATE_TEST_CASE_METHOD(IndexerFixture, "Get all indices", "[indexer]", MutexIndexer, ConcurrentIndexer) {  // NOLINT
    IndexerFixture<TestType>::GetAllIndices();
}

TEMPLATE_TEST_CASE_METHOD(IndexerFixture, "Get and return one index", "[indexer]", MutexIndexer, ConcurrentIndexer) {  // NOLINT
    IndexerFixture<TestType>::GetAndReturnOneIndex();
}

TEMPLATE_TEST_CASE_METHOD(IndexerFixture, "Get and return one index multiple times", "[indexer]", MutexIndexer, ConcurrentIndexer) {  // NOLINT
    IndexerFixture<TestType>::GetAndReturnOneIndexMultipleTimes();
}


TEMPLATE_TEST_CASE_METHOD(IndexerFixture, "Get and return various indices", "[indexer]", MutexIndexer, ConcurrentIndexer) {  // NOLINT
    IndexerFixture<TestType>::GetAndreturnVariousIndices();
}

TEMPLATE_TEST_CASE_METHOD(IndexerFixture, "Get index, no more indices", "[indexer]", MutexIndexer, ConcurrentIndexer) {  // NOLINT
    IndexerFixture<TestType>::GetIndexNoMoreIndices();
}

TEMPLATE_TEST_CASE_METHOD(IndexerFixture, "Get all indices multithreaded", "[indexer]", MutexIndexer, ConcurrentIndexer) {  // NOLINT
    IndexerFixture<TestType>::GetIndicesMultiThreaded();
}

TEMPLATE_TEST_CASE_METHOD(IndexerFixture, "Get and return indices multithreaded", "[indexer]", MutexIndexer, ConcurrentIndexer) {  // NOLINT
    IndexerFixture<TestType>::GetAndReturnIndicesMultiThreaded();
}

TEMPLATE_TEST_CASE_METHOD(IndexerFixture, "Get and return indices more threads than items", "[indexer]", MutexIndexer, ConcurrentIndexer) {  // NOLINT
    IndexerFixture<TestType>::GetAndReturnIndicesMoreThreadsThanItems();
}