#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>

struct UserData;
struct Order;
struct Trade;

// Серверная логика
class Core
{
public:
    // "Регистрирует" нового пользователя и возвращает его ID.
    std::string RegisterNewUser(const std::string& aUserName);

    // Запрос имени клиента по ID
    std::string GetUserName(const std::string& aUserId) const;

    // Запрос баланса клиента по ID
    std::string GetUserBalance(const std::string& aUserId) const;

    // Запрос на добавление новой заявки в стакан
    std::string PlaceNewOrder(const std::string& aUserId,
        const std::string& aAmount,
        const std::string& aPrice,
        bool isBuy);

    // Запрос на вывод активных заявок 
    std::string GetUserActiveQuotes(const std::string& aUserId) const;

    // Запрос на вывод истории сделок
    std::string GetUserTrades(const std::string& aUserId) const;

    // Запрос на удаление активной заявки
    std::string CancelUserQuote(const std::string& aUserId, const std::string& aQuote);

private:
    std::map<size_t, UserData> mUsers;
    std::vector<Order> mBuyOrders;
    std::vector<Order> mSellOrders;
    std::vector<Trade> mTrades;
    std::mutex mMutex;

private:
    void MatchOrder(Order& order, std::vector<Order>& opp);
};

struct UserData
{
  std::string name;
  double usd;
  double rub;
};

struct Order
{
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
    // в приоритете более ранние заявки
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

struct Trade
{
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
    os << t.sellerId << " SOLD "
       << t.buyerId << ' '
       << t.amount << " USD for "
       << t.price << " RUB";

    return os;
  }

};
