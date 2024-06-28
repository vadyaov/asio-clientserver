#include <cstdlib>
#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>
#include "json.hpp"
#include "Common.hpp"

#include <mutex>
#include <chrono>

using boost::asio::ip::tcp;

struct Order {
  std::string userId;
  double amount;
  double price;
  bool isBuy;

  std::chrono::steady_clock::time_point timepoint;

  Order(const std::string& id, double am, double pr, bool buy)
    : userId{id}, amount{am}, price{pr}, isBuy{buy}, timepoint{std::chrono::steady_clock::now()}
  {
  }

  bool operator<(const Order& other) {
    if (price == other.price)
    {
      return timepoint > other.timepoint;
    }

    if (isBuy)
    {
      return price < other.price;
    }
    
    return price > other.price;
  }

  friend std::ostream& operator<<(std::ostream& os, const Order& order) {
    os << order.userId << ' ' << order.amount << ' ' << order.price <<
      (order.isBuy ? " BUY" : " SELL");

    return os;
  }
};

struct Trade {
  std::string buyerId;
  std::string sellerId;
  double amount;
  double price;
  std::chrono::steady_clock::time_point timepoint;

  Trade(const std::string& buyer, const std::string& seller, double am, double pr)
    : buyerId{buyer}, sellerId{seller}, amount{am}, price{pr}, timepoint{std::chrono::steady_clock::now()}
  {
  }

  friend std::ostream& operator<<(std::ostream& os, const Trade& t)
  {
    os << t.buyerId << std::setw(7)
       << t.sellerId << std::setw(7)
       << t.amount << std::setw(7)
       << t.price;

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

    // Пополняет RUB баланс пользователя
    std::string TopUpBalance(const std::string& aUserId, const std::string& amount) {
      const auto userIt = mUsers.find(std::stoi(aUserId));
      if (userIt == mUsers.cend())
      {
        return "Error! Unknown User";
      }
      else
      {
        userIt->second.rub += std::stod(amount);
        return "Success!\n";
      }
    }

    // Получает баланс пользователя
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
      Order newOrder(aUserId, std::stod(aAmount), std::stod(aPrice), isBuy);

      if (isBuy)
      {
        MatchOrder(newOrder, mSellOrders);
        if (newOrder.amount > 0)
        {
          mBuyOrders.push_back(newOrder);
          std::push_heap(mBuyOrders.begin(), mBuyOrders.end());
        }
      }
      else
      {
        MatchOrder(newOrder, mBuyOrders);
        if (newOrder.amount > 0)
        {
          mSellOrders.push_back(newOrder);
          std::push_heap(mSellOrders.begin(), mSellOrders.end());
        }
      }

      return "Your order was succesfully placed.\n";
    }

    std::string ShowMarketDepth() const {
      if (mBuyOrders.empty() && mSellOrders.empty())
      {
        return "Market Depth is empty.\n";
      }

      std::stringstream ss;

      for (const auto& o : mBuyOrders)
      {
        ss << o << '\n';
      }

      if (!mSellOrders.empty()) ss << '\n';
      for (const auto& o : mSellOrders)
      {
        ss << o << '\n';
      }

      return ss.str();
    }

    std::string ShowTrades() const
    {
      if (mTrades.empty())
      {
        return "Trade history is empty.\n";
      }

      std::stringstream ss;
      ss << "BUYER  SELLER  AMOUNT  PRICE\n";
      for (const Trade& t : mTrades)
      {
        ss << t << '\n';
      }

      return ss.str();
    }

private:
    std::map<size_t, UserData> mUsers;
    std::vector<Order> mBuyOrders;
    std::vector<Order> mSellOrders;
    std::vector<Trade> mTrades;
    std::mutex mMutex;

private:
    void MatchOrder(Order& order, std::vector<Order>& opp)
    {
      std::vector<Order> tempOrders;

      auto orderUser = mUsers.find(std::stoi(order.userId));
      std::make_heap(opp.begin(), opp.end());

      while (order.amount > 0 && !opp.empty())
      {
        Order& topOrder = opp.front();

        if (topOrder.userId == order.userId)
        {
          tempOrders.push_back(topOrder);
          std::pop_heap(opp.begin(), opp.end());
          opp.pop_back();
          continue;
        }

        if (!doMatch(order, topOrder)) break;

        auto topOrderUser = mUsers.find(std::stoi(topOrder.userId));
        makeMatch(orderUser, topOrderUser, order, topOrder);

        if (topOrder.amount == 0)
        {
          std::pop_heap(opp.begin(), opp.end());
          opp.pop_back();
        }
      }

      for (Order& ordr : tempOrders)
      {
        opp.push_back(ordr);
        std::push_heap(opp.begin(), opp.end());
      }
    }

    bool doMatch(const Order& left, const Order& right) const
    {
      return (left.isBuy && left.price >= right.price) ||
            (!left.isBuy && left.price <= right.price);
    }

    void makeMatch(std::map<size_t, UserData>::iterator aUser1,
                   std::map<size_t, UserData>::iterator aUser2,
                   Order& aOrder1,
                   Order& aOrder2)
    {
      double tradeAmount = std::min(aOrder1.amount, aOrder2.amount);
      double tradePrice = getTradePrice(aOrder1, aOrder2);
      double tradeTotalPrice = tradeAmount * tradePrice;

      std::string buyerId, sellerId;

      aOrder1.amount -= tradeAmount;
      aOrder2.amount -= tradeAmount;
      if (aOrder1.isBuy)
      {
        buyerId = std::to_string(aUser1->first);
        sellerId = std::to_string(aUser2->first);

        aUser1->second.rub -= tradeTotalPrice;
        aUser1->second.usd += tradeAmount;

        aUser2->second.rub += tradeTotalPrice;
        aUser2->second.usd -= tradeAmount;
      }
      else
      {
        buyerId = std::to_string(aUser2->first);
        sellerId = std::to_string(aUser1->first);

        aUser1->second.rub += tradeTotalPrice;
        aUser1->second.usd -= tradeAmount;

        aUser2->second.rub -= tradeTotalPrice;
        aUser2->second.usd += tradeAmount;
      }

      Trade trade(buyerId, sellerId, tradeAmount, tradePrice);
      mTrades.push_back(trade);
    }

    double getTradePrice(const Order& left, const Order& right) const
    {
      double price;
      if (left.timepoint == right.timepoint)
      {
        price = std::max(left.price, right.price);
      }
      else if (left.timepoint < right.timepoint)
      {
        price = left.price;
      }
      else
      {
        price = right.price;
      }

      return price;
    }
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
            else if (reqType == Requests::QuotesInfo)
            {
              reply = GetCore().ShowMarketDepth();
            }
            else if (reqType == Requests::Trades)
            {
              reply = GetCore().ShowTrades();
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
