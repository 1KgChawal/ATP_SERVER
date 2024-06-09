#include "exchange.h"

boost::asio::io_context io_context;

using boost::asio::ip::tcp;
std::unordered_map<long long, std::shared_ptr<tcp::socket>> client_socket_mapping;
std::unordered_map<std::shared_ptr<tcp::socket>, long long> socket_client_mapping;

tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8888));
long long next_client_id = 1;

void start_accept();
void read_handler(const boost::system::error_code &ec, std::size_t len,
                  char *buffer, std::shared_ptr<tcp::socket> sock);

void write_handler(const boost::system::error_code &ec, std::size_t len,
                   std::shared_ptr<tcp::socket> sock);

void handle_order(const order &ord);
void assign_client_socket(std::shared_ptr<tcp::socket> sock);
void display(int x, long long q, order &buy_top, order &sell_top, long long client_buy, long long client_sell);

void handle_order(const order &ord)
{
    add_to_priority_queue(ord);
}

void assign_client_socket(std::shared_ptr<tcp::socket> sock, long long clientID)
{
    if (client_socket_mapping.find(clientID) == client_socket_mapping.end())
    {
        client_socket_mapping.insert({clientID, sock});
        socket_client_mapping.insert({sock, clientID});
        return;
    }
    client_socket_mapping[clientID] = sock;
    socket_client_mapping.insert({sock, clientID});
    return;
}

void write_handler(const boost::system::error_code &ec, std::size_t len,
                   std::shared_ptr<tcp::socket> sock)
{
    char *buf = new char[1024];
    memset(buf, 0, 1024);
    sock->async_read_some(
        boost::asio::buffer(buf, 1024),
        boost::bind(read_handler, boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred, buf,
                    std::shared_ptr<tcp::socket>(sock)));
}

void read_handler(const boost::system::error_code &ec, std::size_t len,
                  char *buffer, std::shared_ptr<tcp::socket> sock)
{
    if (!ec)
    {
        std::string data(buffer, buffer + len);
        if (data.find("-1") != std::string::npos)
        {
            assign_client_socket(sock, next_client_id++);
        }
        else
        {
            bool num = true;
            for (auto &x : data)
            {
                if (x < '0' || x > '9')
                {
                    num = false;
                    break;
                }
            }
            if (num)
            {
                long long ID = 0;
                for (auto &x : data)
                {
                    ID *= 10;
                    ID += (x - '0');
                }
                assign_client_socket(sock, ID);
            }
            else
            {
                std::cout << "Received data from client " << socket_client_mapping[sock] << " : " << buffer << std::endl;
                std::shared_ptr<char[]> buffer_copy(new char[len]);
                std::memcpy(buffer_copy.get(), buffer, len);
                order ord = parse(buffer_copy, socket_client_mapping[sock]);
                
                std::ostringstream message;
                if(ord.type==OrderType::ERR){message<<"INVALID FORMAT\n";}
                else{
                message << " Order of Order ID: " << ord.order_id << " with quantity " << ord.quantity << " and price " << ord.price << " has been registered at " << ord.time.count() << "\n";
                }
                std::string message_str = message.str();
                sock->async_write_some(
                    boost::asio::buffer(message_str.c_str(), message_str.length()),
                    boost::bind(write_handler, boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                sock));
                if(ord.type!=OrderType::ERR)handle_order(ord);
            }
        }
        char *new_buffer = new char[1024];
        sock->async_read_some(
            boost::asio::buffer(new_buffer, 1024),
            boost::bind(read_handler, boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred, new_buffer,
                        std::shared_ptr<tcp::socket>(sock)));
    }
    else
    {
        std::cerr << "Error in read_handler: " << ec.message() << std::endl;
    }
}

void display(int x, long long q, order &buy_top, order &sell_top, long long client_buy, long long client_sell)
{
    std::ostringstream message_buy, message_sell;
    message_buy << " Order ID: " << buy_top.order_id << "\n";
    if (x == 0)
        message_buy << "Partial ";
    message_buy << " Buy Order executed!\n";
    long long q1 = buy_top.original_quantity;
    if (x == 0)
        q1 = q;
    message_buy << " Quantity Bought: " << q << " at Price:" << sell_top.price << "\n";

    message_sell << " Order ID: " << sell_top.order_id << "\n";
    if (x == 1)
        message_sell << "Partial ";
    message_sell << "Sell Order executed!\n";
    long long q2 = sell_top.original_quantity;
    if (x == 1)
        q2 = q;
    message_sell << " Quantity Sold: " << q << " at Price:" << sell_top.price << "\n";

    std::shared_ptr<tcp::socket> socket_buy_client = client_socket_mapping[client_buy];
    std::shared_ptr<tcp::socket> socket_sell_client = client_socket_mapping[client_sell];

    std::string m_buy = message_buy.str();
    std::string m_sell = message_sell.str();
    // std::cout << m_buy << "\n" << m_sell << "\n";
    // Define completion handlers
    auto buy_write_handler = [socket_buy_client](const boost::system::error_code &error, size_t bytes_transferred)
    {
        if (!error)
        {
            std::cout << "Data sent to buy client\n";
        }
        else
        {
            std::cerr << "Error sending data to buy client: " << error.message() << "\n";
        }
    };

    auto sell_write_handler = [socket_sell_client](const boost::system::error_code &error, size_t bytes_transferred)
    {
        if (!error)
        {
            std::cout << "Data sent to sell client\n";
        }
        else
        {
            std::cerr << "Error sending data to sell client: " << error.message() << "\n";
        }
    };

    boost::asio::async_write(*socket_buy_client,
                             boost::asio::buffer(m_buy.c_str(), m_buy.length()),
                             buy_write_handler);

    boost::asio::async_write(*socket_sell_client,
                             boost::asio::buffer(m_sell.c_str(), m_sell.length()),
                             sell_write_handler);
}

void accept_handler(const boost::system::error_code &ec,
                    std::shared_ptr<tcp::socket> sock)
{
    if (!ec)
    {
        char *buf = new char[1024];
        sock->async_read_some(
            boost::asio::buffer(buf, 1024),
            boost::bind(read_handler, boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred, buf,
                        std::shared_ptr<tcp::socket>(sock)));
    }
    else
    {
        std::cerr << "Error in accept_handler: " << ec.message() << std::endl;
    }
    start_accept();
}

void start_accept()
{
    std::shared_ptr<tcp::socket> conn(new tcp::socket(io_context));
    acceptor.async_accept(
        *conn, boost::bind(accept_handler, boost::asio::placeholders::error,
                           std::shared_ptr<tcp::socket>(conn)));
}

int main(int argc, char *argv[])
{
    std::cout << boost::asio::ip::address_v4::loopback() << "\n";
    start_accept();
    io_context.run();
    return 0;
}
