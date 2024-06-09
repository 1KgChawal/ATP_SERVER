#ifndef EXCHANGE_GUARD
#define EXCHANGE_GUARD

#include <cstdint>
#include <memory>
#include <queue>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/bind/bind.hpp>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <fstream>
#include <typeinfo>

enum class OrderType
{
    BUY = 0,
    SELL = 1,
    ERR=2
};

struct order
{
    int64_t order_id, quantity, original_quantity;
    std::chrono::milliseconds time;
    long double price;
    OrderType type;
    bool operator<(const order &other) const
    {
        if (price != other.price)
        {
            return (price < other.price) ^ (type == OrderType::SELL);
        }
        else
        {
            return time > other.time;
        }
    }
};

order parse(std::shared_ptr<char[]> buffer, long long clientID);

void add_to_priority_queue(const order &ord);

void match();

void display(int x, long long q, order &buy_top, order &sell_top, long long client_buy, long long client_sell);

extern std::priority_queue<order> BUY_PQ, SELL_PQ;
extern long long order_id;
#endif
