# MatchingEngine

   Emphasis should be placed on developing code that is both highly efficient AND maintainable.



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

  struct Order

  {

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



    uint64_t  order_id_;  // Some globally unique identifier for this order, where it comes from is not important

    int64_t   price_;     // Some normalized price type, details aren't important

    uint32_t  quantity_;  // Number of shares to buy or sell

    Side      side_;      // Whether order is to buy or sell

    OrderType type_;      // The order type, limit or market



    // ... Add any additional members you want

  };



  class Fill

  {

    // Define this class

  };



  class Reject

  {

    // Define this class

  };



  class Cancel

  {

    // Define this class

  };



  class OrderAck

  {

    // Define this class

  }



  struct MessageHub

  {

    // You need to call these functions to notify the system of

    // fills, rejects, cancels, and order acknowledgements

    virtual void SendFill(Fill&) = 0;     // Call twice per fill, once for each order that participated in the fill (i.e. the buy and sell orders).

    virtual void SendReject(Reject&) = 0; // Call when a 'market' order can't be filled immediately

    virtual void SendCancel(Cancel&) = 0; // Call when an order is successfully cancelled

    virtual void SendOrderAck(OrderAck&) = 0;// Call when a 'limit' order doesn't trade immediately, but is placed into the book

  };



  class MatchingEngine

  {

    MessageHub* message_hub_;

    // Add any additional members you want



  public:



    MatchingEngine(MessageHub* message_hub)

      : message_hub_(message_hub)

    {}



    // Implement these functions, and any other supporting functions or classes needed.

    // You can assume these functions will be called by external code, and that they will be used

    // properly, i.e the order objects are valid and filled out correctly.

    // You should call the message_hub_ member to notify it of the various events that happen as a result

    // of matching orders and entering orders into the order book.

    void submit_new_order(Order& order);

    void cancel_existing_order(Order& order);



    // Add any additional methods you want



  };
