#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main()
{
    try
    {
        boost::asio::io_context io_context;

        std::string server_ip = "127.0.0.1";
        unsigned short server_port = 8888;

        tcp::socket socket(io_context);

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(server_ip, std::to_string(server_port));

        boost::asio::connect(socket, endpoints);

        std::cout << "Connected to the server." << std::endl;

        std::string clientID = "-1";
        boost::asio::write(socket, boost::asio::buffer(clientID));

        std::thread([&]()
                    {
            try {
                while (true) {
                    char response[1024];
                    size_t bytes_transferred = socket.read_some(boost::asio::buffer(response));
                    if (bytes_transferred > 0) {
                        std::cout << "Server response: " << std::string(response, response + bytes_transferred) << std::endl;
                    }
                }
            } catch (const std::exception &e) {
                std::cerr << "Error reading from server: " << e.what() << std::endl;
            } })
            .detach();

        while (true)
        {
            std::string user_input;
            std::getline(std::cin, user_input);

            if (user_input == "exit")
            {
                break;
            }

            boost::asio::write(socket, boost::asio::buffer(user_input + "\n"));
        }

        socket.close();
        std::cout << "Disconnected from the server." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
