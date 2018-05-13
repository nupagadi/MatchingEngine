#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "MatchingEngine.hpp"


using ::testing::_;
using ::testing::InSequence;

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

TEST_F(Tests, ShouldCancelRemainigMarketWhenNotEnough)
{
    InSequence s;

    Matching::Fill f;
    Matching::Cancel c;

    Matching::Order l1{1, 123, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_));
    Engine.SubmitNewOrder(l1);

    Matching::Order m1{2, 1111, 101, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Market};
    f.order_id_ = 2;
    f.quantity_ = 100;
    f.price_ = 123;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 1;
    EXPECT_CALL(MessageHub, SendFill(f));
    c.order_id_ = 2;
    EXPECT_CALL(MessageHub, SendCancel(c));
    Engine.SubmitNewOrder(m1);


    Matching::Order l21{21, 132, 30, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_));
    Engine.SubmitNewOrder(l21);
    Matching::Order l22{22, 100, 40, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_));
    Engine.SubmitNewOrder(l22);

    Matching::Order m2{4, 1, 101, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Market};
    f.order_id_ = 4;
    f.quantity_ = 40;
    f.price_ = 100;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 22;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 4;
    f.quantity_ = 30;
    f.price_ = 132;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 21;
    EXPECT_CALL(MessageHub, SendFill(f));
    c.order_id_ = 4;
    EXPECT_CALL(MessageHub, SendCancel(c));
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

TEST_F(Tests, ShouldFillMarketWithTwoLimits)
{
    InSequence s;

    Matching::Fill f;

    Matching::Order l11{11, 123, 40, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_)).Times(2);
    Engine.SubmitNewOrder(l11);
    Matching::Order l12{12, 130, 60, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    Engine.SubmitNewOrder(l12);

    Matching::Order m1{2, 123, 100, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Market};
    f.order_id_ = 2;
    f.quantity_ = 60;
    f.price_ = 130;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 12;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 2;
    f.quantity_ = 40;
    f.price_ = 123;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 11;
    EXPECT_CALL(MessageHub, SendFill(f));
    Engine.SubmitNewOrder(m1);
}

TEST_F(Tests, ShouldFillUpLimit)
{
    InSequence s;

    Matching::Fill f;

    Matching::Order l11{11, 123, 40, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_)).Times(2);
    Engine.SubmitNewOrder(l11);
    Matching::Order l12{12, 130, 60, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    Engine.SubmitNewOrder(l12);

    Matching::Order l13{13, 123, 90, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    f.order_id_ = 13;
    f.quantity_ = 60;
    f.price_ = 130;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 12;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 13;
    f.quantity_ = 30;
    f.price_ = 123;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 11;
    EXPECT_CALL(MessageHub, SendFill(f));
    Engine.SubmitNewOrder(l13);

    Matching::Order m1{3, 123, 90, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Market};
    f.order_id_ = 3;
    f.quantity_ = 10;
    f.price_ = 123;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 11;
    EXPECT_CALL(MessageHub, SendFill(f));
    Matching::Cancel c1{3};
    EXPECT_CALL(MessageHub, SendCancel(c1));
    Engine.SubmitNewOrder(m1);


    Matching::Order l21{21, 140, 40, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_)).Times(2);
    Engine.SubmitNewOrder(l21);
    Matching::Order l22{22, 140, 60, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    Engine.SubmitNewOrder(l22);

    Matching::Order l23{23, 140, 90, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    f.order_id_ = 23;
    f.quantity_ = 40;
    f.price_ = 140;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 21;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 23;
    f.quantity_ = 50;
    f.price_ = 140;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 22;
    EXPECT_CALL(MessageHub, SendFill(f));
    Engine.SubmitNewOrder(l23);

    Matching::Order m2{4, 123, 90, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Market};
    f.order_id_ = 4;
    f.quantity_ = 10;
    f.price_ = 140;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 22;
    EXPECT_CALL(MessageHub, SendFill(f));
    Matching::Cancel c2{4};
    EXPECT_CALL(MessageHub, SendCancel(c2));
    Engine.SubmitNewOrder(m2);
}

TEST_F(Tests, ShouldPartiallyFillLimitThenAck)
{
    InSequence s;

    Matching::Fill f;

    Matching::Order l11{11, 130, 40, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_)).Times(2);
    Engine.SubmitNewOrder(l11);
    Matching::Order l12{12, 130, 60, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    Engine.SubmitNewOrder(l12);

    Matching::Order l13{13, 123, 110, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    f.order_id_ = 13;
    f.quantity_ = 40;
    f.price_ = 130;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 11;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 13;
    f.quantity_ = 60;
    f.price_ = 130;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 12;
    EXPECT_CALL(MessageHub, SendFill(f));
    Matching::OrderAck a1{13};
    EXPECT_CALL(MessageHub, SendOrderAck(a1));
    Engine.SubmitNewOrder(l13);

    Matching::Order m1{3, 123, 90, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Market};
    f.order_id_ = 3;
    f.quantity_ = 10;
    f.price_ = 123;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 13;
    EXPECT_CALL(MessageHub, SendFill(f));
    Matching::Cancel c1{3};
    EXPECT_CALL(MessageHub, SendCancel(c1));
    Engine.SubmitNewOrder(m1);


    Matching::Order l21{21, 140, 40, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_)).Times(2);
    Engine.SubmitNewOrder(l21);
    Matching::Order l22{22, 140, 60, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    Engine.SubmitNewOrder(l22);

    Matching::Order l23{23, 140, 120, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    f.order_id_ = 23;
    f.quantity_ = 40;
    f.price_ = 140;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 21;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 23;
    f.quantity_ = 60;
    f.price_ = 140;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 22;
    EXPECT_CALL(MessageHub, SendFill(f));
    Matching::OrderAck a2{23};
    EXPECT_CALL(MessageHub, SendOrderAck(a2));
    Engine.SubmitNewOrder(l23);

    Matching::Order m2{4, 123, 90, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Market};
    f.order_id_ = 4;
    f.quantity_ = 20;
    f.price_ = 140;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 23;
    EXPECT_CALL(MessageHub, SendFill(f));
    Matching::Cancel c2{4};
    EXPECT_CALL(MessageHub, SendCancel(c2));
    Engine.SubmitNewOrder(m2);
}

TEST_F(Tests, ShouldNotAddOrdersWithSameId)
{
    Matching::Reject r{1, "Id already exists."};

    Matching::Order o1{1, 123, 100, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    Matching::Order o2{1, 123, 100, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    Matching::Order o3{1, 123, 10, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Market};
    Matching::Order o4{1, 123, 10, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Market};

    EXPECT_CALL(MessageHub, SendOrderAck(_));
    Engine.SubmitNewOrder(o1);
    EXPECT_CALL(MessageHub, SendReject(r));
    Engine.SubmitNewOrder(o2);
    EXPECT_CALL(MessageHub, SendReject(r));
    Engine.SubmitNewOrder(o3);
    EXPECT_CALL(MessageHub, SendReject(r));
    Engine.SubmitNewOrder(o4);
}

TEST_F(Tests, ShouldCancelOrder)
{
    Matching::Reject r{1, "Id not found."};

    InSequence s;

    Matching::Fill f;

    Matching::Order l11{11, 130, 40, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    EXPECT_CALL(MessageHub, SendOrderAck(_)).Times(2);
    Engine.SubmitNewOrder(l11);
    Matching::Order l12{12, 130, 60, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Limit};
    Engine.SubmitNewOrder(l12);

    Matching::Cancel c1{11};
    EXPECT_CALL(MessageHub, SendCancel(c1));
    Matching::Order o1{11};
    Engine.CancelExistingOrder(o1);
    r.order_id_ = 11;
    EXPECT_CALL(MessageHub, SendReject(r));
    Engine.CancelExistingOrder(o1);

    Matching::Order l13{13, 123, 110, 0, Matching::Order::Side::Sell, Matching::Order::OrderType::Limit};
    f.order_id_ = 13;
    f.quantity_ = 60;
    f.price_ = 130;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 12;
    EXPECT_CALL(MessageHub, SendFill(f));
    Matching::OrderAck a1{13};
    EXPECT_CALL(MessageHub, SendOrderAck(a1));
    Engine.SubmitNewOrder(l13);

    Matching::Order m1{3, 123, 90, 0, Matching::Order::Side::Buy, Matching::Order::OrderType::Market};
    f.order_id_ = 3;
    f.quantity_ = 50;
    f.price_ = 123;
    EXPECT_CALL(MessageHub, SendFill(f));
    f.order_id_ = 13;
    EXPECT_CALL(MessageHub, SendFill(f));
    Matching::Cancel c2{3};
    EXPECT_CALL(MessageHub, SendCancel(c2));
    Engine.SubmitNewOrder(m1);
}


int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

