#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MatchingEngine.hpp"


using ::testing::_;

struct MessageHubMock : Matching::MessageHub
{
    MOCK_METHOD1(SendFill, void(Matching::Fill&));
    MOCK_METHOD1(SendReject, void(Matching::Reject&));
    MOCK_METHOD1(SendCancel, void(Matching::Cancel&));
    MOCK_METHOD1(SendOrderAck, void(Matching::OrderAck&));
};

struct Tests : ::testing::Test
{
    ::testing::StrictMock<MessageHubMock> MessageHub;

    Matching::MatchingEngine Engine{&MessageHub};

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(Tests, ShouldAckLimit)
{
    Matching::OrderAck oa;

    Matching::Order o1{1, 123, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};

    oa.order_id_ = 1;
    EXPECT_CALL(MessageHub, SendOrderAck(oa));

    Engine.SubmitNewOrder(o1);


    Matching::Order o2{2, 132, 100, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};

    oa.order_id_ = 2;
    EXPECT_CALL(MessageHub, SendOrderAck(oa));

    Engine.SubmitNewOrder(o2);


    Matching::Order o3{3, 120, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};

    oa.order_id_ = 3;
    EXPECT_CALL(MessageHub, SendOrderAck(oa));

    Engine.SubmitNewOrder(o3);


    Matching::Order o4{4, 130, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};

    oa.order_id_ = 4;
    EXPECT_CALL(MessageHub, SendOrderAck(oa));

    Engine.SubmitNewOrder(o4);
}

TEST_F(Tests, ShouldRejectMarketWhenEmpty)
{
    Matching::Reject r{0, "Not enough liquidity."};

    Matching::Order o1{1, 123, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Market};

    r.order_id_ = 1;
    EXPECT_CALL(MessageHub, SendReject(r));

    Engine.SubmitNewOrder(o1);


    Matching::Order o2{2, 132, 100, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Market};

    r.order_id_ = 2;
    EXPECT_CALL(MessageHub, SendReject(r));

    Engine.SubmitNewOrder(o2);


    Matching::Order o3{3, 120, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Market};

    r.order_id_ = 3;
    EXPECT_CALL(MessageHub, SendReject(r));

    Engine.SubmitNewOrder(o3);
}

TEST_F(Tests, ShouldRejectMarketWhenNotEnough)
{
    Matching::Reject r{0, "Not enough liquidity."};

    Matching::Order l1{1, 123, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_));
    Engine.SubmitNewOrder(l1);

    Matching::Order m1{2, 123, 101, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Market};
    r.order_id_ = 2;
    EXPECT_CALL(MessageHub, SendReject(r));
    Engine.SubmitNewOrder(m1);


    Matching::Order l2{3, 132, 100, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_));
    Engine.SubmitNewOrder(l2);

    Matching::Order m2{4, 132, 101, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Market};
    r.order_id_ = 4;
    EXPECT_CALL(MessageHub, SendReject(r));
    Engine.SubmitNewOrder(m2);
}

TEST_F(Tests, ShouldFillMarketWhenEnough)
{
    Matching::Fill f;

    Matching::Order l1{1, 123, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_));
    Engine.SubmitNewOrder(l1);

    Matching::Order m1{2, 123, 100, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Market};
    f.order_id_ = 1;
    f.quantity_ = 100;
    f.price_ = 123;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 2;
    EXPECT_CALL(MessageHub, SendFill(f));
    Engine.SubmitNewOrder(m1);


    Matching::Order l2{3, 132, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_));
    Engine.SubmitNewOrder(l2);

    Matching::Order m2{4, 132, 100, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Market};
    f.order_id_ = 3;
    f.quantity_ = 100;
    f.price_ = 132;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 4;
    EXPECT_CALL(MessageHub, SendFill(f));
    Engine.SubmitNewOrder(m2);
}

// ShouldFillByExistingOrderPrice
// ShouldBeRemovedAfterFill
// ShouldAcceptOrderWithLeavesQty
// ShouldNotOverFillOrder

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

