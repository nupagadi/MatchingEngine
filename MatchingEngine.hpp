// Your job is to implement a Matching Engine and the supporting classes to implement a simple exchange.
// The matching engine is just for a single symbol, say "IBM", so there is no need to complicate it beyond that.
// The Matching Engine should support Limit and Market orders being submitted, and Limit orders may also be cancelled
// if they aren't already filled.  Market orders should be either immediately filled, cancelled, or partially filled
// and the remainder cancelled.  They should never rest on the book.  Limit orders should fill any qty they can on
// being submitted, and the remainder of the order should rest on the appropriate side of the book until another order
// comes in that can trade with it.  The order book of resting orders should be maintained in price/time priority
// similar to how most US exchanges work.  When a new order comes in that could possibly trade with multiple resting
// orders, the order at the "best" price should be chosen, and if multiple orders are available at the best price, the
// oldest order should be chosen.  If the new order is bigger in quantity than a resting order, it should continue to
// fill additional resting orders until it is completely filled, or there are no additional resting orders on that side.
// If it is not completely filled, the remaining qty should rest on the book at the order limit price.  Example:
//
// submit_new_order ( order id 1, quantity 100, price 50, side sell, type limit) -> results in
//   Order Ack (order id 1, shares remaining 100)
//
// submit_new_order ( order id 2, quantity 100, price 50, side sell, type Market) -> results in
//   Order Reject (order id 2) //reject because there are no sell orders for it to trade with, only buy orders
//
// submit_new_order ( order id 3, quantity 100, price 49, side sell, type limit) -> results in
//   Order Ack (order id 3, shares remaining 100)
//
// submit_new_order ( order id 4, quantity 100, price 50, side sell, type limit) -> results in
//   Order Ack (order id 4, shares remaining 100)
//
// submit_new_order ( order id 5, quantity 100, price 48, side buy, type limit) -> results in
//   Order Ack (order id 5, shares remaining 100)
//
// The order book now looks something like this:
//   Price Level:  Resting Order Ids:  Side:
//     50:             1, 4            sell
//     49:             3               sell
//     48:             5               buy
//
// submit_new_order ( order id 6, quantity 200, price 51, side buy, type limit) -> results in
//   Fill (order id 3, shares 100, price 49, side sell)
//   Fill (order id 6, shares 100, price 49, side buy)
//   Fill (order id 1, shares 100, price 50, side sell)
//   Fill (order id 6, shares 100, price 50, side buy)
//
// The order book now looks something like this:
//   Price Level:  Resting Order Ids:  Side:
//     50:             4               sell
//     48:             5               buy
//
// submit_new_order ( order id 7, quantity 200, price 51, side buy, type limit) -> results in
//   Fill (order id 4, shares 100, price 50, side sell)
//   Fill (order id 7, shares 100, price 50, side buy)
//   Order Ack (order id 7, shares remaining 100)
//
// The order book now looks something like this:
//   Price Level:  Resting Order Ids:    Side:
//     51:             7 (100 remaining) buy
//     48:             5                 buy
//
// cancel_existing_order ( order id 5 ) -> results in
//   Cancel (order id 5)
//
// The order book now looks something like this:
//   Price Level:  Resting Order Ids:    Side:
//     51:             7 (100 remaining) buy
//


// This code has a few C++11 constructs, feel free to change to C++98 if needed.


// Assume this class is constructed for you, you don't need to construct it
// or manage its lifetime.

#pragma once

#include <map>
#include <functional>

namespace Matching
{

struct Order
{
    using TPrice = int64_t;
    using TQty = uint32_t;
    using TOrderId = uint64_t;

    enum class Side : char
    {
        Buy,
        Sell
    };

    enum class OrderType : char
    {
        Market,            // Trade at any price available, reject order if it can't trade immediately
        Limit              // Trade at the specified price or better, rest order on book if it can't trade immediately
    };

    TOrderId  order_id_;  // Some globally unique identifier for this order, where it comes from is not important
    TPrice    price_;     // Some normalized price type, details aren't important
    TQty      quantity_;  // Number of shares to buy or sell
    TQty      cum_qty_;   // Number of shares already filled
    Side      side_;      // Whether order is to buy or sell
    OrderType type_;      // The order type, limit or market
};

// Assume that the order in which messages are delivered to users is guaranteed to be the same as their sending.
// So, for example, there is no need to specify leaves quantity for partially filled market order (in Cancel)
// as it can be determined from previously sent Fill orders.

struct Fill
{
    Order::TOrderId order_id_;
    Order::TQty     quantity_;
    Order::TPrice   price_;

    bool operator==(const Fill& fill) const
    {
        return order_id_ == fill.order_id_ && quantity_ == fill.quantity_ && price_ == fill.price_;
    }
};

struct Reject
{
    Order::TOrderId order_id_;
    std::string     message_;

    bool operator==(const Reject& reject) const
    {
        return order_id_ == reject.order_id_ && message_ == reject.message_;
    }

};

struct Cancel
{
    Order::TOrderId order_id_;

    bool operator==(const Cancel& cancel) const
    {
        return order_id_ == cancel.order_id_;
    }

};

struct OrderAck
{
    Order::TOrderId order_id_;

    bool operator==(const OrderAck& ack) const
    {
        return order_id_ == ack.order_id_;
    }

};


struct MessageHub
{
    // You need to call these functions to notify the system of
    // fills, rejects, cancels, and order acknowledgements

    virtual void SendFill(Fill&) = 0;     // Call twice per fill, once for each order that participated in the fill (i.e. the buy and sell orders).

    virtual void SendReject(Reject&) = 0; // Call when a 'market' order can't be filled immediately

    virtual void SendCancel(Cancel&) = 0; // Call when an order is successfully cancelled

    virtual void SendOrderAck(OrderAck&) = 0;// Call when a 'limit' order doesn't trade immediately, but is placed into the book
};

template <Order::Side TSide>
using Better = std::conditional_t<TSide == Order::Side::Buy, std::greater<Order::TPrice>, std::less<Order::TPrice>>;

class MatchingEngine
{
    // std::multimap keeps entries with the same key in order of their insertion.
    template <typename TBetter>
    using TMap = std::multimap<Order::TPrice, Order, TBetter>;

    using TBids = TMap<Better<Order::Side::Buy>>;
    using TAcks = TMap<Better<Order::Side::Sell>>;

    template <typename TThisSide, typename TOtherSide>
    struct Templates
    {
        TThisSide& This;
        TOtherSide& Other;
    };

private:

    static const constexpr char* NotEnoughLiquidityMessage = "Not enough liquidity.";

    MessageHub* message_hub_;
    TBids bids_;
    TAcks asks_;

public:

    MatchingEngine(MessageHub* message_hub)
        : message_hub_(message_hub)
    {}

    // Implement these functions, and any other supporting functions or classes needed.
    // You can assume these functions will be called by external code, and that they will be used
    // properly, i.e the order objects are valid and filled out correctly.
    // You should call the message_hub_ member to notify it of the various events that happen as a result
    // of matching orders and entering orders into the order book.

    void SubmitNewOrder(Order& order)
    {
        order.cum_qty_ = 0;

        if (order.side_ == Order::Side::Buy)
        {
            // Derermine the sides only once.
            Templates<TBids, TAcks> temp{bids_, asks_,};
            ProcessOrder(order, temp);
        }
        else if (order.side_ == Order::Side::Sell)
        {
            Templates<TAcks, TBids> temp{asks_, bids_,};
            ProcessOrder(order, temp);
        }
    }

    void CancelExistingOrder(Order& order)
    {

    }

private:

    template <typename T>
    void ProcessOrder(Order& order, T& templates)
    {
        if (order.type_ == Order::OrderType::Market && templates.Other.empty())
        {
            Reject r{order.order_id_, NotEnoughLiquidityMessage};
            message_hub_->SendReject(r);
            return;
        }

        while (MatchOne(order, templates.Other));

        if (order.cum_qty_ != order.quantity_)
        {
            if (order.type_ == Order::OrderType::Limit)
            {
                templates.This.emplace(order.price_, order);

                OrderAck ack{order.order_id_};
                message_hub_->SendOrderAck(ack);
            }
            else if (order.type_ == Order::OrderType::Market)
            {
                Cancel cancel{order.order_id_};
                message_hub_->SendCancel(cancel);
            }
        }
    }

    template <typename T>
    Order::TQty MatchOne(Order& order, T& other_side)
    {
        if (other_side.empty())
        {
            return 0;
        }

        if (order.quantity_ - order.cum_qty_ == 0)
        {
            return 0;
        }

        auto& first_order = other_side.begin()->second;
        if (!typename T::key_compare()(order.price_, first_order.price_) && order.type_ == Order::OrderType::Limit
            || order.type_ == Order::OrderType::Market)
        {
            auto min = std::min(order.quantity_ - order.cum_qty_, first_order.quantity_ - first_order.cum_qty_);

            Fill(order, min, first_order.price_);
            Fill(first_order, min, first_order.price_);

            if (first_order.quantity_ == first_order.cum_qty_)
            {
                other_side.erase(other_side.begin());
            }

            return min;
        }

        return 0;
    }

    void Fill(Order& order, Order::TQty qty, Order::TPrice price)
    {
        assert(order.quantity_ - order.cum_qty_ >= qty);

        order.cum_qty_ += qty;

        Matching::Fill f{order.order_id_, qty, price};
        message_hub_->SendFill(f);
    }
};

}
