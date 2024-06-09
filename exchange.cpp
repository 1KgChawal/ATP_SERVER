#include "exchange.h"

std::priority_queue<order> BUY_PQ, SELL_PQ;
std::unordered_map<long long, long long> order_client_mapping;

order create_error_order()
{
    order err_order;
    err_order.type = OrderType::ERR;
    return err_order;
}

long long order_id = 1;
order parse(std::shared_ptr<char[]> buffer, long long clientID)
{
    std::istringstream stream(buffer.get());
    order ord;
    std::string type_str;

    if (!(stream >> type_str >> ord.price >> ord.quantity))
    {
     //throw std::runtime_error("Error parsing order data");
    return create_error_order();
    }
    if (type_str == "BUY")
    {
        ord.type = OrderType::BUY;
    }
    else if (type_str == "SELL")
    {
        ord.type = OrderType::SELL;
    }
    else
    {
       // throw std::runtime_error("Invalid order type: " + type_str);
         return create_error_order();
    }
    ord.original_quantity = ord.quantity;
    ord.time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
    ord.order_id = order_id;
    order_client_mapping.insert({order_id, clientID});
    order_id++;
    return ord;
}

void add_to_priority_queue(const order &ord)
{
    if (ord.type == OrderType::BUY)
    {
        BUY_PQ.push(ord);
    }
    else
    {
        SELL_PQ.push(ord);
    }
    match();
}

void match()
{
    while (!BUY_PQ.empty() && !SELL_PQ.empty())
    {

        order buy_top = const_cast<order &>(BUY_PQ.top());
        order sell_top = const_cast<order &>(SELL_PQ.top());
        std::cout << buy_top.order_id << " " << sell_top.order_id << std::endl;

        if (buy_top.price < sell_top.price)
        {
            break;
        }
        if (buy_top.quantity > sell_top.quantity)
        {
            display(0, sell_top.quantity, buy_top, sell_top, order_client_mapping[buy_top.order_id], order_client_mapping[sell_top.order_id]);
            SELL_PQ.pop();
	    BUY_PQ.pop(); 
            buy_top.quantity -= sell_top.quantity;
	    BUY_PQ.push(buy_top);
        }
        else if (buy_top.quantity < sell_top.quantity)
        {
            display(1, buy_top.quantity, buy_top, sell_top, order_client_mapping[buy_top.order_id], order_client_mapping[sell_top.order_id]);
            BUY_PQ.pop();
	    SELL_PQ.pop();
            sell_top.quantity -= buy_top.quantity;
              SELL_PQ.push(sell_top);
	}
        else
        {
            display(2, buy_top.quantity, buy_top, sell_top, order_client_mapping[buy_top.order_id], order_client_mapping[sell_top.order_id]);
            BUY_PQ.pop();
            SELL_PQ.pop();
        }
    }
}
