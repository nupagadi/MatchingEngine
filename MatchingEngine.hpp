#pragma once

#include <map>
#include <unordered_map>
#include <functional>

namespace Matching
{

// Assume that the order in which messages are delivered to users is guaranteed to be the same as their sending.
// So, for example, there is no need to specify leaves quantity for partially filled market order (in Cancel)
// as it can be determined from previously sent Fill orders.

// Possibly, there should be distinct client order id and engine-side order id,
// so different clients could have same ids of their orders.

// There are two instructions, which seems to contradict each others:
// 1. Market orders should be either immediately filled, cancelled, or partially filled and the remainder cancelled.
// 2. Trade at any price available, reject order if it can't trade immediately.
// This engine implemented in a way where market orders rejected when there is no any liquidity
// or if there is, it is partially filled, then canceled.

struct Order
{
    using TPrice =   int64_t;
    using TQty =     uint32_t;
    using TOrderId = uint64_t;

    enum class Side : char
    {
        Buy,
        Sell
    };

    enum class OrderType : char
    {
        Market,
        Limit
    };

    TOrderId  order_id_;
    TPrice    price_;
    TQty      quantity_;
    TQty      cum_qty_;   // Number of shares already filled
    Side      side_;
    OrderType type_;
};

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
    virtual void SendFill(Fill&) = 0;

    virtual void SendReject(Reject&) = 0;

    virtual void SendCancel(Cancel&) = 0;

    virtual void SendOrderAck(OrderAck&) = 0;
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
        TThisSide&  This;
        TOtherSide& Other;
    };

private:

    static const constexpr char* NotEnoughLiquidityMessage  = "Not enough liquidity.";
    static const constexpr char* IdAlreadyExistsMessage     = "Id already exists.";
    static const constexpr char* IdNotFoundMessage          = "Id not found.";

    MessageHub* message_hub_;

    TBids       bids_;
    TAcks       asks_;
    std::unordered_map<Order::TOrderId, TBids::iterator> ids_;

public:

    MatchingEngine(MessageHub* message_hub)
        : message_hub_(message_hub)
    {}

    void SubmitNewOrder(Order& order)
    {
        auto res = ids_.find(order.order_id_);
        if (res != ids_.cend())
        {
            Reject r{order.order_id_, IdAlreadyExistsMessage};
            message_hub_->SendReject(r);
            return;
        }

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
        auto res = ids_.find(order.order_id_);
        if (res == ids_.cend())
        {
            Reject r{order.order_id_, IdNotFoundMessage};
            message_hub_->SendReject(r);
            return;
        }

        auto& order_to_cancel = res->second->second;
        if (order_to_cancel.side_ == Order::Side::Buy)
        {
            bids_.erase(res->second);
            ids_.erase(order.order_id_);

            Cancel cancel{order.order_id_};
            message_hub_->SendCancel(cancel);
        }
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
                auto it = templates.This.emplace(order.price_, order);

                ids_.emplace(order.order_id_, it);

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
                ids_.erase(order.order_id_);
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
