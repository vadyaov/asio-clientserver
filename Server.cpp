#include <cstdlib>
#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "json.hpp"
#include "Common.hpp"

#include <mutex>

using boost::asio::ip::tcp;

struct Order {
  std::string userId;
  double amount;
  double price;
  bool isBuy;

  bool operator<(const Order& other) {
    if (isBuy)
    {
      return price < other.price;
    }
    
    return price > other.price;
  }

  friend std::ostream& operator<<(std::ostream& os, const Order& order) {
    os << order.userId << ' ' << order.amount << ' ' << order.price <<
      (order.isBuy ? "BUY" : "SELL") << '\n';

    return os;
  }
};

struct UserData {
  std::string name;
  double usd;
  double rub;
};

// Серверная логика
class Core
{
public:
    // "Регистрирует" нового пользователя и возвращает его ID.
    std::string RegisterNewUser(const std::string& aUserName)
    {
        size_t newUserId = mUsers.size();
        mUsers[newUserId].name = aUserName;
        mUsers[newUserId].usd = 0;
        mUsers[newUserId].rub = 0;

        return std::to_string(newUserId);
    }

    // Запрос имени клиента по ID
    std::string GetUserName(const std::string& aUserId)
    {
        const auto userIt = mUsers.find(std::stoi(aUserId));
        if (userIt == mUsers.cend())
        {
            return "Error! Unknown User";
        }
        else
        {
            return userIt->second.name;
        }
    }

    // вывод данных всех клиентов в формате ID: name
    std::string GetUsersData() const {
      if (mUsers.empty()) return "No users on server";

      std::string data;
      for (const auto& [id, userData] : mUsers) {
        data += std::to_string(id);
        data += ": ";
        data += userData.name;
        data += "\n";
      }

      return data;
    }

    std::string TopUpBalance(const std::string& aUserId, const std::string& amount) {
      const auto userIt = mUsers.find(std::stoi(aUserId));
      if (userIt == mUsers.cend())
      {
        return "Error! Unknown User";
      }
      else
      {
        userIt->second.rub += std::stod(amount);
        return "Success!\n"; // + GetBalance(std::to_string(userIt->first));
      }
    }

    std::string GetBalance(const std::string& aUserId) const {
      const auto userIt = mUsers.find(std::stoi(aUserId));
      if (userIt == mUsers.cend())
      {
        return "Error! Unknown User";
      }
      else
      {
        std::string rubles {std::to_string(userIt->second.rub)};
        std::string dollas {std::to_string(userIt->second.usd)};
        rubles.erase(rubles.find_last_not_of('0') + 1, std::string::npos);
        rubles.erase(rubles.find_last_not_of('.') + 1, std::string::npos);
        dollas.erase(dollas.find_last_not_of('0') + 1, std::string::npos);
        dollas.erase(dollas.find_last_not_of('.') + 1, std::string::npos);
        return "RUB " + rubles + "\n" +
               "USD " + dollas + "\n";
      }
    }

    std::string PlaceOrder(const std::string& aUserId,
                           const std::string& aAmount,
                           const std::string& aPrice,
                           bool isBuy)
    {
      std::lock_guard<std::mutex> lock(mMutex);
      Order newOrder = {aUserId, std::stod(aAmount), std::stod(aPrice), isBuy};

      if (isBuy)
      {
        mBuyOrders.push_back(newOrder);
        std::push_heap(mBuyOrders.begin(), mBuyOrders.end());
      }
      else
      {
        mSellOrders.push_back(newOrder);
        std::push_heap(mSellOrders.begin(), mSellOrders.end());
      }

      return "Your order was succesfully placed.\n" + DumpOrders();
    }

    std::string DumpOrders() const {
      std::stringstream ss;
      for (const auto& o : mBuyOrders)
      {
        ss << o;
      }

      for (const auto& o : mSellOrders)
      {
        ss << o;
      }

      return ss.str();
    }

private:
    // <UserId, UserName>
    std::map<size_t, UserData> mUsers;
    std::vector<Order> mBuyOrders;
    std::vector<Order> mSellOrders;
    std::mutex mMutex;

};

Core& GetCore()
{
    static Core core;
    return core;
}

// обработка клиентских сессий
// Класс обрабатывает входящие сообщения от клиента и отправляет ответы
class session
{
public:
    session(boost::asio::io_service& io_service)
        : socket_(io_service)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
            boost::bind(&session::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

    // Обработка полученного сообщения.
    void handle_read(const boost::system::error_code& error,
        size_t bytes_transferred)
    {
        if (!error)
        {
            data_[bytes_transferred] = '\0';

            // Парсим json, который пришёл нам в сообщении.
            auto j = nlohmann::json::parse(data_);
            auto reqType = j["ReqType"];

            std::string reply = "Error! Unknown request type";
            if (reqType == Requests::Registration)
            {
                // Это реквест на регистрацию пользователя.
                // Добавляем нового пользователя и возвращаем его ID.
                reply = GetCore().RegisterNewUser(j["Message"]);
            }
            else if (reqType == Requests::TopUpBalance)
            {
                // Это реквест на выставление пополнение баланса RUB.
                // Находим имя пользователя по ID и пополняем его RUB баланс.
                reply = GetCore().TopUpBalance(j["UserId"], j["Message"]);
            }
            else if (reqType == Requests::Balance)
            {
              // Это реквест на вывод баланса пользователя
                reply = GetCore().GetBalance(j["UserId"]);
            }
            else if (reqType == Requests::BuyOrder)
            {
              std::string message = j["Message"];
              auto order = nlohmann::json::parse(message);
              reply = GetCore().PlaceOrder(j["UserId"], order["Amount"], order["Price"], true);
            }
            else if(reqType == Requests::SellOrder)
            {
              std::string message = j["Message"];
              auto order = nlohmann::json::parse(message);
              reply = GetCore().PlaceOrder(j["UserId"], order["Amount"], order["Price"], false);
            }

            boost::asio::async_write(socket_,
                boost::asio::buffer(reply, reply.size()),
                boost::bind(&session::handle_write, this,
                    boost::asio::placeholders::error));
        }
        else
        {
            delete this;
        }
    }

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                boost::bind(&session::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            delete this;
        }
    }

private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

// Управление сервером
// Создает новые сессии для каждого нового подключения
class server
{
public:
    server(boost::asio::io_service& io_service)
        : io_service_(io_service),
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
    {
        std::cout << "Server started! Listen " << port << " port" << std::endl;

        session* new_session = new session(io_service_);
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&server::handle_accept, this, new_session,
                boost::asio::placeholders::error));
    }

    void handle_accept(session* new_session,
        const boost::system::error_code& error)
    {
        if (!error)
        {
            new_session->start();
            new_session = new session(io_service_);
            acceptor_.async_accept(new_session->socket(),
                boost::bind(&server::handle_accept, this, new_session,
                    boost::asio::placeholders::error));
        }
        else
        {
            delete new_session;
        }
    }

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
};

int main()
{
    try
    {
        boost::asio::io_service io_service;
        static Core core;

        server s(io_service);

        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
