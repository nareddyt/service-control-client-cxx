// TODO: Insert description here. (generated by jaebong)

#include "src/quota_aggregator_impl.h"

#include <unordered_map>

#include "gmock/gmock.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "utils/status_test_util.h"

#include <unistd.h>

using std::string;
using ::google::api::servicecontrol::v1::QuotaOperation;
using ::google::api::servicecontrol::v1::AllocateQuotaRequest;
using ::google::api::servicecontrol::v1::AllocateQuotaResponse;
using ::google::api::servicecontrol::v1::QuotaOperation_QuotaMode;
using ::google::protobuf::TextFormat;
using ::google::protobuf::util::MessageDifferencer;
using ::google::protobuf::util::Status;
using ::google::protobuf::util::error::Code;

namespace google {
namespace service_control_client {
namespace {

const char kServiceName[] = "library.googleapis.com";
const char kServiceConfigId[] = "2016-09-19r0";

const int kFlushIntervalMs = 100;
const int kExpirationMs = 400;

const char kRequest1[] = R"(
service_name: "library.googleapis.com"
allocate_operation {
  operation_id: "operation-1"
  method_name: "methodname"
  consumer_id: "consumerid"
  quota_metrics {
    metric_name: "metric_first"
    metric_values {
      int64_value: 1
    }
  }
  quota_metrics {
    metric_name: "metric_second"
    metric_values {
      int64_value: 1
    }
  }
  quota_mode: BEST_EFFORT
}
service_config_id: "2016-09-19r0"
)";

const char kRequest2[] = R"(
service_name: "library.googleapis.com"
allocate_operation {
  operation_id: "operation-2"
  method_name: "methodname2"
  consumer_id: "consumerid2"
  quota_metrics {
    metric_name: "metric_first"
    metric_values {
      int64_value: 2
    }
  }
  quota_metrics {
    metric_name: "metric_second"
    metric_values {
      int64_value: 3
    }
  }
  quota_mode: BEST_EFFORT
}
service_config_id: "2016-09-19r0"
)";

const char kRequest3[] = R"(
service_name: "library.googleapis.com"
allocate_operation {
  operation_id: "operation-3"
  method_name: "methodname3"
  consumer_id: "consumerid3"
  quota_metrics {
    metric_name: "metric_first"
    metric_values {
      int64_value: 1
    }
  }
  quota_metrics {
    metric_name: "metric_second"
    metric_values {
      int64_value: 1
    }
  }
  quota_mode: BEST_EFFORT
}
service_config_id: "2016-09-19r0"
)";

const char kSuccessResponse1[] = R"(
operation_id: "operation-1"
quota_metrics {
  metric_name: "serviceruntime.googleapis.com/api/consumer/quota_used_count"
  metric_values {
    labels {
      key: "/quota_name"
      value: "metric_first"
    }
    int64_value: 1
  }
  metric_values {
    labels {
      key: "/quota_name"
      value: "metric_first"
    }
    int64_value: 1
  }
}
service_config_id: "2016-09-19r0"
)";

const char kErrorResponse1[] = R"(
operation_id: "a197c6f2-aecc-4a31-9744-b1d5aea4e4b4"
allocate_errors {
  code: RESOURCE_EXHAUSTED
  subject: "user:integration_test_user"
}
)";

const char kSuccessResponse2[] = R"(
operation_id: "550e8400-e29b-41d4-a716-446655440000"
quota_metrics {
  metric_name: "serviceruntime.googleapis.com/api/consumer/quota_used_count"
  metric_values {
    labels {
      key: "/quota_name"
      value: "metric_first"
    }
    int64_value: 1
  }
  metric_values {
    labels {
      key: "/quota_name"
      value: "metric_second"
    }
    int64_value: 1
  }
}
service_config_id: "2017-02-08r5"
)";

const char kErrorResponse2[] = R"(
operation_id: "a197c6f2-aecc-4a31-9744-b1d5aea4e4b4"
allocate_errors {
  code: RESOURCE_EXHAUSTED
  subject: "user:integration_test_user"
}
)";

const char kEmptyResponse[] = R"(
)";

const std::set<std::pair<std::string, int>> ExtractMetricSets(
    const ::google::api::servicecontrol::v1::QuotaOperation& operation) {
  std::set<std::pair<std::string, int>> sets;

  for (auto quota_metric : operation.quota_metrics()) {
    sets.insert(std::make_pair(quota_metric.metric_name(),
                               quota_metric.metric_values(0).int64_value()));
  }

  return sets;
}

}  // namespace

class QuotaAggregatorImplTest : public ::testing::Test {
 public:
  void SetUp() {
    ASSERT_TRUE(TextFormat::ParseFromString(kRequest1, &request1_));
    ASSERT_TRUE(TextFormat::ParseFromString(kRequest2, &request2_));
    ASSERT_TRUE(TextFormat::ParseFromString(kRequest3, &request3_));

    ASSERT_TRUE(
        TextFormat::ParseFromString(kSuccessResponse1, &pass_response1_));
    ASSERT_TRUE(
        TextFormat::ParseFromString(kSuccessResponse2, &pass_response2_));

    ASSERT_TRUE(
        TextFormat::ParseFromString(kErrorResponse1, &error_response1_));
    ASSERT_TRUE(
        TextFormat::ParseFromString(kErrorResponse2, &error_response2_));

    ASSERT_TRUE(TextFormat::ParseFromString(kEmptyResponse, &empty_response_));

    QuotaAggregationOptions options(10, kFlushIntervalMs, kExpirationMs);

    aggregator_ =
        CreateAllocateQuotaAggregator(kServiceName, kServiceConfigId, options);
    ASSERT_TRUE((bool)(aggregator_));

    aggregator_->SetFlushCallback(std::bind(
        &QuotaAggregatorImplTest::FlushCallback, this, std::placeholders::_1));
  }

  void FlushCallback(const AllocateQuotaRequest& request) {
    flushed_.push_back(request);
  }

  void FlushCallbackCallingBackToAggregator(
      const AllocateQuotaRequest& request) {
    flushed_.push_back(request);
    (void)aggregator_->CacheResponse(request, pass_response1_);
  }

  AllocateQuotaRequest request1_;
  AllocateQuotaRequest request2_;
  AllocateQuotaRequest request3_;

  AllocateQuotaResponse pass_response1_;
  AllocateQuotaResponse pass_response2_;

  AllocateQuotaResponse error_response1_;
  AllocateQuotaResponse error_response2_;

  AllocateQuotaResponse empty_response_;

  std::unique_ptr<QuotaAggregator> aggregator_;
  std::vector<AllocateQuotaRequest> flushed_;
};

TEST_F(QuotaAggregatorImplTest, TestRequestAndCache) {
  AllocateQuotaResponse response;

  EXPECT_OK(aggregator_->Quota(request1_, &response));
  EXPECT_TRUE(MessageDifferencer::Equals(response, empty_response_));

  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  EXPECT_OK(aggregator_->Quota(request1_, &response));
  EXPECT_TRUE(MessageDifferencer::Equals(response, pass_response1_));
}

TEST_F(QuotaAggregatorImplTest, TestCacheElementStay) {
  AllocateQuotaResponse response;

  EXPECT_OK(aggregator_->Quota(request1_, &response));
  EXPECT_TRUE(MessageDifferencer::Equals(response, empty_response_));

  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  std::this_thread::sleep_for(std::chrono::milliseconds(kExpirationMs - 10));
  EXPECT_OK(aggregator_->Flush());

  EXPECT_OK(aggregator_->Quota(request1_, &response));
  EXPECT_TRUE(MessageDifferencer::Equals(response, pass_response1_));
}

TEST_F(QuotaAggregatorImplTest, TestCacheElementDrop) {
  AllocateQuotaResponse response;

  EXPECT_OK(aggregator_->Quota(request1_, &response));
  EXPECT_TRUE(MessageDifferencer::Equals(response, empty_response_));

  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  std::this_thread::sleep_for(std::chrono::milliseconds(kExpirationMs + 10));
  EXPECT_OK(aggregator_->Flush());

  EXPECT_OK(aggregator_->Quota(request1_, &response));
  EXPECT_TRUE(MessageDifferencer::Equals(response, empty_response_));
}


TEST_F(QuotaAggregatorImplTest, TestNotMatchingServiceName) {
  *(request1_.mutable_service_name()) = "some-other-service-name";
  AllocateQuotaResponse response;

  EXPECT_ERROR_CODE(Code::INVALID_ARGUMENT,
                    aggregator_->Quota(request1_, &response));
}

TEST_F(QuotaAggregatorImplTest, TestNoOperation) {
  request1_.clear_allocate_operation();
  AllocateQuotaResponse response;

  EXPECT_ERROR_CODE(Code::INVALID_ARGUMENT,
                    aggregator_->Quota(request1_, &response));
}

TEST_F(QuotaAggregatorImplTest, TestFlushAggregatedRecord) {
  AllocateQuotaResponse response;

  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response));
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));
  EXPECT_OK(aggregator_->Quota(request1_, &response));
  EXPECT_TRUE(MessageDifferencer::Equals(response, pass_response1_));

  EXPECT_EQ(flushed_.size(), 1);

  // simulate refresh timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(110));

  EXPECT_OK(aggregator_->Flush());
  EXPECT_EQ(flushed_.size(), 2);
}

TEST_F(QuotaAggregatorImplTest, TestFlushedBeforeRefreshTimeout) {
  AllocateQuotaResponse response;

  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response));
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));
  EXPECT_OK(aggregator_->Quota(request1_, &response));
  EXPECT_TRUE(MessageDifferencer::Equals(response, pass_response1_));

  EXPECT_OK(aggregator_->Flush());

  // The request 1 remaining in the cache is not yet flushed.
  EXPECT_EQ(flushed_.size(), 1);
}

TEST_F(QuotaAggregatorImplTest, TestInflightCache) {
  AllocateQuotaResponse response;

  // temporary cached with in_flight flag on
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response));

  // hit cached response and aggregate
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response));

  std::this_thread::sleep_for(std::chrono::milliseconds(110));
  EXPECT_OK(aggregator_->Flush());

  // hit cached response and aggregate
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response));

  std::this_thread::sleep_for(std::chrono::milliseconds(110));
  EXPECT_OK(aggregator_->Flush());

  // nothing refreshed
  EXPECT_EQ(flushed_.size(), 1);

  // update the response in the cache and set in_flight flag off
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  // hit cached response and aggregate
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response));

  std::this_thread::sleep_for(std::chrono::milliseconds(110));

  EXPECT_OK(aggregator_->Flush());

  // one element added to the refresh list
  EXPECT_EQ(flushed_.size(), 2);
}

TEST_F(QuotaAggregatorImplTest, TestCacheAggregateAfterRefreshAndCacheUpdate) {
  std::set<std::pair<std::string, int>> quota_metrics;
  std::set<std::pair<std::string, int>> expected_costs;

  AllocateQuotaResponse response1;
  AllocateQuotaResponse response2;

  // insert request1
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response1));
  EXPECT_EQ(flushed_.size(), 1);
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // insert request2
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request2_, &response2));
  EXPECT_OK(aggregator_->CacheResponse(request2_, pass_response2_));

  // aggregate request1
  EXPECT_OK(aggregator_->Quota(request1_, &response1));

  // aggregate request2
  EXPECT_OK(aggregator_->Quota(request2_, &response2));

  // verify responses from cache
  EXPECT_TRUE(MessageDifferencer::Equals(response1, pass_response1_));
  EXPECT_TRUE(MessageDifferencer::Equals(response2, pass_response2_));

  // check flushed out list is empty
  EXPECT_EQ(flushed_.size(), 2);

  std::this_thread::sleep_for(std::chrono::milliseconds(60));

  // expire request1
  EXPECT_OK(aggregator_->Flush());
  EXPECT_EQ(flushed_.size(), 3);

  EXPECT_TRUE(flushed_[0].has_allocate_operation());
  quota_metrics = ExtractMetricSets(flushed_[0].allocate_operation());
  expected_costs = {{"metric_first", 1}, {"metric_second", 1}};
  ASSERT_EQ(expected_costs, quota_metrics);

  std::this_thread::sleep_for(std::chrono::milliseconds(60));

  // expire request2
  EXPECT_OK(aggregator_->Flush());
  EXPECT_EQ(flushed_.size(), 4);

  EXPECT_TRUE(flushed_[1].has_allocate_operation());
  expected_costs = {{"metric_first", 2}, {"metric_second", 3}};
  quota_metrics = ExtractMetricSets(flushed_[1].allocate_operation());
  ASSERT_EQ(quota_metrics, expected_costs);

  // reset in_flight flag for request1
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response2_));

  // expire temporary elements for refreshment from the cache
  std::this_thread::sleep_for(std::chrono::milliseconds(110));
  EXPECT_OK(aggregator_->Flush());
  EXPECT_EQ(flushed_.size(), 4);

  // lookup request should be Code::OK. Element will not be expired.
  AllocateQuotaResponse response3;
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response3));
}

TEST_F(QuotaAggregatorImplTest,
       TestCacheAggregateAfterRefreshBeforeCacheUpdate) {
  std::set<std::pair<std::string, int>> quota_metrics;
  std::set<std::pair<std::string, int>> expected_costs;

  AllocateQuotaResponse response1;

  // insert request1 to cache and removed item
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response1));
  EXPECT_EQ(flushed_.size(), 1);

  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  // response has arrived
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));
  std::this_thread::sleep_for(std::chrono::milliseconds(30));

  // aggregated to request1
  EXPECT_OK(aggregator_->Quota(request1_, &response1));
  EXPECT_EQ(flushed_.size(), 1);

  std::this_thread::sleep_for(std::chrono::milliseconds(60));

  // expire request1, temporary 1 inserted
  EXPECT_OK(aggregator_->Flush());
  EXPECT_EQ(flushed_.size(), 2);
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  // aggregated to temporary three times
  EXPECT_OK(aggregator_->Quota(request1_, &response1));
  EXPECT_OK(aggregator_->Quota(request1_, &response1));
  EXPECT_OK(aggregator_->Quota(request1_, &response1));

  // elapse time to trigger refresh by Quota
  std::this_thread::sleep_for(std::chrono::milliseconds(110));

  EXPECT_OK(aggregator_->Quota(request1_, &response1));
  EXPECT_EQ(flushed_.size(), 3);
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  std::this_thread::sleep_for(std::chrono::milliseconds(110));

  // send refresh for expired item
  EXPECT_OK(aggregator_->Flush());

  // check
  EXPECT_EQ(flushed_.size(), 4);
  // response has arrived, set in_flight flag off
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  // temporary 1 will be flushed
  EXPECT_EQ(flushed_.size(), 4);

  // check first flushed
  EXPECT_TRUE(flushed_[1].has_allocate_operation());
  expected_costs = {{"metric_first", 1}, {"metric_second", 1}};
  quota_metrics = ExtractMetricSets(flushed_[1].allocate_operation());
  ASSERT_EQ(quota_metrics, expected_costs);

  // check second flushed
  EXPECT_TRUE(flushed_[2].has_allocate_operation());
  expected_costs = {{"metric_first", 3}, {"metric_second", 3}};
  quota_metrics = ExtractMetricSets(flushed_[2].allocate_operation());
  ASSERT_EQ(quota_metrics, expected_costs);

  // check second flushed
  EXPECT_TRUE(flushed_[3].has_allocate_operation());
  expected_costs = {{"metric_first", 1}, {"metric_second", 1}};
  quota_metrics = ExtractMetricSets(flushed_[3].allocate_operation());
  ASSERT_EQ(quota_metrics, expected_costs);
}

TEST_F(QuotaAggregatorImplTest, TestCacheRefreshOneAggregated) {
  std::set<std::pair<std::string, int>> quota_metrics;
  std::set<std::pair<std::string, int>> expected_costs;

  AllocateQuotaResponse response1;
  AllocateQuotaResponse response2;

  // insert request 1
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response1));
  EXPECT_EQ(flushed_.size(), 1);
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // insert request 2
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request2_, &response2));
  EXPECT_EQ(flushed_.size(), 2);
  EXPECT_OK(aggregator_->CacheResponse(request2_, pass_response2_));

  // aggregate request 1
  EXPECT_OK(aggregator_->Quota(request1_, &response1));

  // check responses from cache
  EXPECT_TRUE(MessageDifferencer::Equals(response1, pass_response1_));
  EXPECT_TRUE(MessageDifferencer::Equals(response2, empty_response_));

  std::this_thread::sleep_for(std::chrono::milliseconds(60));

  // expire request1
  EXPECT_OK(aggregator_->Flush());
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  // check request1 has flushed out
  EXPECT_EQ(flushed_.size(), 3);

  // aggregate request 2
  EXPECT_OK(aggregator_->Quota(request2_, &response2));
  EXPECT_OK(aggregator_->Quota(request2_, &response2));

  std::this_thread::sleep_for(std::chrono::milliseconds(60));

  // expire request2
  EXPECT_OK(aggregator_->Flush());
  EXPECT_OK(aggregator_->CacheResponse(request2_, pass_response2_));

  // the second cached item did not have aggregated quota, when it is flushed,
  // it was dropped.
  EXPECT_EQ(flushed_.size(), 4);

  // verify flushed out 1 again
  expected_costs = {{"metric_first", 1}, {"metric_second", 1}};
  quota_metrics = ExtractMetricSets(flushed_[2].allocate_operation());
  ASSERT_EQ(quota_metrics, expected_costs);

  // verify flushed out 1 again
  expected_costs = {{"metric_first", 4}, {"metric_second", 6}};
  quota_metrics = ExtractMetricSets(flushed_[3].allocate_operation());
  ASSERT_EQ(quota_metrics, expected_costs);
}

TEST_F(QuotaAggregatorImplTest, TestCacheRefreshQuotaModeBestEffort) {
  std::set<std::pair<std::string, int>> quota_metrics;
  std::set<std::pair<std::string, int>> expected_costs;

  AllocateQuotaResponse response;

  // trigger initial allocation
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response));
  // check refresh
  EXPECT_EQ(flushed_.size(), 1);
  // cache positive response
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // aggregate
  EXPECT_OK(aggregator_->Quota(request1_, &response));

  // trigger refresh request by timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  EXPECT_OK(aggregator_->Flush());

  // check refresh request
  EXPECT_EQ(flushed_.size(), 2);
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  EXPECT_EQ(flushed_[0].allocate_operation().quota_mode(),
            QuotaOperation_QuotaMode::QuotaOperation_QuotaMode_BEST_EFFORT);
  EXPECT_EQ(flushed_[1].allocate_operation().quota_mode(),
            QuotaOperation_QuotaMode::QuotaOperation_QuotaMode_BEST_EFFORT);
}

TEST_F(QuotaAggregatorImplTest, TestCacheRefreshQuotaModeNormal) {
  std::set<std::pair<std::string, int>> quota_metrics;
  std::set<std::pair<std::string, int>> expected_costs;

  AllocateQuotaResponse response;

  // trigger initial allocation
  EXPECT_ERROR_CODE(Code::OK, aggregator_->Quota(request1_, &response));
  // check refresh
  EXPECT_EQ(flushed_.size(), 1);
  // cache negative response
  EXPECT_OK(aggregator_->CacheResponse(request1_, error_response1_));

  // keep request 1 without refresh
  std::this_thread::sleep_for(std::chrono::milliseconds(110));
  EXPECT_OK(aggregator_->Flush());
  // check no refresh
  EXPECT_EQ(flushed_.size(), 1);

  // trigger refresh request 1
  EXPECT_OK(aggregator_->Quota(request1_, &response));

  // check refresh
  EXPECT_EQ(flushed_.size(), 2);
  EXPECT_OK(aggregator_->CacheResponse(request1_, pass_response1_));

  EXPECT_EQ(flushed_[0].allocate_operation().quota_mode(),
            QuotaOperation_QuotaMode::QuotaOperation_QuotaMode_BEST_EFFORT);
  EXPECT_EQ(flushed_[1].allocate_operation().quota_mode(),
            QuotaOperation_QuotaMode::QuotaOperation_QuotaMode_NORMAL);
}


}  // namespace service_control_client
}  // namespace google
