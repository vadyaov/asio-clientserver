#include <iostream>
#include <boost/asio.hpp>

#include "Common.hpp"
#include "json.hpp"

using boost::asio::ip::tcp;

// Отправка сообщения на сервер по шаблону.
void SendMessage(
    tcp::socket& aSocket,
    const std::string& aId,
    const std::string& aRequestType,
    const std::string& aMessage)
{
    nlohmann::json req;
    req["UserId"] = aId;
    req["ReqType"] = aRequestType;
    req["Message"] = aMessage;

    std::string request = req.dump();
    boost::asio::write(aSocket, boost::asio::buffer(request, request.size()));
}

// Возвращает строку с ответом сервера на последний запрос.
std::string ReadMessage(tcp::socket& aSocket)
{
    boost::asio::streambuf b;
    boost::asio::read_until(aSocket, b, "\0");
    std::istream is(&b);
    std::string line(std::istreambuf_iterator<char>(is), {});
    return line;
}

// "Создаём" пользователя, получаем его ID.
std::string ProcessRegistration(tcp::socket& aSocket)
{
    std::string name;
    std::cout << "Hello! Enter your name: ";
    std::cin >> name;

    // Для регистрации Id не нужен, заполним его нулём
    SendMessage(aSocket, "0", Requests::Registration, name);
    return ReadMessage(aSocket);
}

std::string GetOrderData() {
  std::string amount;
  std::cout << "Amount: ";
  std::cin >> amount;

  std::string price;
  std::cout << "Price: ";
  std::cin >> price;

  nlohmann::json order;
  order["Amount"] = amount;
  order["Price"] = price;

  return order.dump();
}

int main()
{
    try
    {
        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        tcp::resolver::query query(tcp::v4(), "127.0.0.1", std::to_string(port));
        tcp::resolver::iterator iterator = resolver.resolve(query);

        tcp::socket s(io_service);
        s.connect(*iterator);

        // Мы предполагаем, что для идентификации пользователя будет использоваться ID.
        // Тут мы "регистрируем" пользователя - отправляем на сервер имя, а сервер возвращает нам ID.
        // Этот ID далее используется при отправке запросов.
        std::string my_id = ProcessRegistration(s);

        while (true)
        {
            // Тут реализовано "бесконечное" меню.
            std::cout << "Menu:\n"
                         "1) Balance\n"
                         "2) Buy\n"
                         "3) Sell\n"
                         "4) Market Depth\n"
                         "5) Trades\n"
                         "6) Cancel Quote\n"
                         "7) Exit\n"
                         << std::endl;

            short menu_option_num;
            std::cin >> menu_option_num;
            switch (menu_option_num)
            {
                case 1:
                {
                    SendMessage(s, my_id, Requests::Balance, "");
                    std::cout << ReadMessage(s);
                    break;
                }
                case 2:
                {
                    std::string order = GetOrderData();
                    SendMessage(s, my_id, Requests::BuyOrder, order);
                    std::cout << ReadMessage(s);
                    break;
                }
                case 3:
                {
                    std::string order = GetOrderData();
                    SendMessage(s, my_id, Requests::SellOrder, order);
                    std::cout << ReadMessage(s);
                    break;
                }
                case 4:
                {
                    SendMessage(s, my_id, Requests::ActiveQuotes, "");
                    std::cout << ReadMessage(s);
                    break;
                }
                case 5:
                {
                  SendMessage(s, my_id, Requests::Trades, "");
                  std::cout << ReadMessage(s);
                  break;
                }
                case 6:
                {
                  SendMessage(s, my_id, Requests::ActiveQuotes, "");
                  std::cout << ReadMessage(s);
                  std::cout << "Choose the quote to decline (-1 to cancel) ";
                  int quote;
                  std::cin >> quote;
                  if (quote != -1)
                  {
                    SendMessage(s, my_id, Requests::Cancel, std::to_string(quote));
                    std::cout << ReadMessage(s);
                  }
                  break;
                }
                case 7:
                {
                    exit(0);
                }
                default:
                {
                    std::cout << "Unknown menu option\n" << std::endl;
                    break;
                }
            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
