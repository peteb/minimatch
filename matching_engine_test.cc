#include "gtest/gtest.h"
#include "framework/services.h"
#include "matcher/matcher.h"
#include <vector>
#include <map>

class MatchingEngineTest : public testing::Test {
  static std::map<void *, std::vector<order_match_report>> match_reports;

public:
  matching_engine_t *matcher;
  matching_engine_callback_t callback;

  MatchingEngineTest() {
    matcher_init();
    match_reports.clear();
    matcher = static_cast<matching_engine_t *>(find_service("matcher"));
    callback.order_matched = order_matched;
  }

  ~MatchingEngineTest() {
    matcher_shutdown();
  }

  void ASSERT_SUCCESS(status_t status) {
    ASSERT_TRUE(SUCCESS(status));
  }

  void register_callback(const char *ins_id, void *opaque) {
    match_reports.insert({opaque, {}});
    matcher->register_callback(ins_id, opaque, &callback);
  }

  const std::vector<order_match_report_t> &fetch_match_reports(void *opaque) {
    auto iter = match_reports.find(opaque);
    if (iter != match_reports.end()) {
      return iter->second;
    }

    std::abort();
  }

private:
  static status_t order_matched(void *opaque, order_match_report_t *report) {
    auto iter = match_reports.find(opaque);
    if (iter != match_reports.end()) {
      iter->second.push_back(*report);
    }
    else {
      // We're not expecting this callback
      std::abort();
    }

    return SUC_OK;
  }
};

std::map<void *, std::vector<order_match_report>> MatchingEngineTest::match_reports;

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

static const order_match_report_t &expect_report(const std::vector<order_match_report_t> &reports, uint64_t order_id) {
  for (auto &r : reports) {
    if (r.order_id == order_id) {
      return r;
    }
  }

  std::abort();
}

TEST_F(MatchingEngineTest, DecInPriceShouldBe2) {
  int dec = -1;
  ASSERT_SUCCESS(matcher->dec_in_price("blah", &dec));
  ASSERT_EQ(dec, 2);
}

TEST_F(MatchingEngineTest, OrderIsPutInOrderBook) {
  uint64_t order_id = 0;
  ASSERT_EQ(matcher->limit_order("INS123", SIDE_BUY, 10, 1500, &order_id), SUC_INBOOK);
}

TEST_F(MatchingEngineTest, OrderIsExecutedDirectly) {
  // Given
  int listener;
  register_callback("INS123", &listener);

  uint64_t order_buy_id = 0;
  ASSERT_EQ(matcher->limit_order("INS123", SIDE_BUY, 10, 1500, &order_buy_id), SUC_INBOOK);

  // When
  uint64_t order_sell_id = 0;
  ASSERT_EQ(matcher->limit_order("INS123", SIDE_SELL, 10, 1500, &order_sell_id), SUC_EXECUTED);

  // Then
  ASSERT_NE(order_buy_id, 0);
  ASSERT_EQ(order_sell_id, 0);

  auto reports = fetch_match_reports(&listener);
  ASSERT_EQ(reports.size(), 2);

  auto sell_report = expect_report(reports, order_sell_id);
  auto buy_report = expect_report(reports, order_buy_id);

  ASSERT_EQ(sell_report.quantity, 10);
  ASSERT_EQ(buy_report.quantity, 10);
  ASSERT_EQ(sell_report.price, 1500);
  ASSERT_EQ(buy_report.price, 1500);
}
