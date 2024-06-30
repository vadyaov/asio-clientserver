#include "Core.hpp"

namespace
{
  void removeTrailingZeros(std::string& str)
  {
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    str.erase(str.find_last_not_of('.') + 1, std::string::npos);
  }

  bool doMatch(const Order& left, const Order& right)
  {
    return (left.isBuy && left.price >= right.price) ||
          (!left.isBuy && left.price <= right.price);
  }

  double getTradePrice(const Order& left, const Order& right)
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

  Trade makeTrade(std::map<size_t, UserData>::iterator aUser1,
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

    return Trade(buyerId, sellerId, tradeAmount, tradePrice);
  }
} // namespace

std::string Core::RegisterNewUser(const std::string& aUserName)
{
  size_t newUserId = mUsers.size();
  mUsers[newUserId].name = aUserName;
  mUsers[newUserId].usd = 0;
  mUsers[newUserId].rub = 0;

  return std::to_string(newUserId);
}

std::string Core::GetUserName(const std::string& aUserId) const
{
  const auto userIt = mUsers.find(std::stoi(aUserId));
  if (userIt == mUsers.cend())
  {
      return "Error! Unknown User\n";
  }
  else
  {
      return userIt->second.name;
  }
}

std::string Core::GetUserBalance(const std::string& aUserId) const
{
  const auto userIt = mUsers.find(std::stoi(aUserId));
  if (userIt == mUsers.cend())
  {
    return "Error! Unknown User\n";
  }

  std::string rubles {std::to_string(userIt->second.rub)};
  std::string dollas {std::to_string(userIt->second.usd)};

  removeTrailingZeros(rubles);
  removeTrailingZeros(dollas);

  return "RUB " + rubles + "\n" +
         "USD " + dollas + "\n";
}

std::string Core::PlaceNewOrder(const std::string& aUserId,
    const std::string& aAmount,
    const std::string& aPrice,
    bool isBuy)
{
  if (mUsers.find(std::stoi(aUserId)) == mUsers.cend())
  {
    return "Error! Unknown User\n";
  }
  if (std::stod(aAmount) <= 0)
  {
    return "Error. Incorrect USD amount.\n";
  }
  if (std::stod(aPrice) < 0)
  {
    return "Error. Incorrect USD price.\n";
  }

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

// это приватный метод
void Core::MatchOrder(Order& order, std::vector<Order>& opp)
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
    mTrades.push_back(makeTrade(orderUser, topOrderUser, order, topOrder));

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

std::string Core::GetUserActiveQuotes(const std::string& aUserId) const
{
  const auto userIt = mUsers.find(std::stoi(aUserId));
  if (userIt == mUsers.cend())
  {
    return "Error! Unknown User\n";
  }

  std::stringstream ss;
  int i = 0;
  for (const auto& o : mBuyOrders)
  {
    if (aUserId.empty() || aUserId == o.userId)
    {
      ss << ++i << ") " << o << '\n';
    }
  }

  for (const auto& o : mSellOrders)
  {
    if (aUserId.empty() || aUserId == o.userId)
    {
      ss << ++i << ") " << o << '\n';
    }
  }

  return i == 0 ? "You have no active quotes.\n" : ss.str();
}

std::string Core::GetUserTrades(const std::string& aUserId) const
{
  const auto userIt = mUsers.find(std::stoi(aUserId));
  if (userIt == mUsers.cend())
  {
    return "Error! Unknown User\n";
  }

  int trades {0};
  std::stringstream ss;
  for (const Trade& t : mTrades)
  {
    if (t.buyerId == aUserId || t.sellerId == aUserId)
    {
      ss << t << '\n';
      ++trades;
    }
  }

  return 0 == trades ? "You have no completed trades.\n" : ss.str();
}

std::string Core::CancelUserQuote(const std::string& aUserId, const std::string& aQuote)
{
  const auto userIt = mUsers.find(std::stoi(aUserId));
  if (userIt == mUsers.cend())
  {
    return "Error! Unknown User\n";
  }
  
  int quote = std::stoi(aQuote);
  if (quote < 1)
  {
    return "Incorrect Quote number.\n";
  }

  std::lock_guard<std::mutex> lock(mMutex);

  for (auto it = mBuyOrders.begin(); it != mBuyOrders.end(); ++it)
  {
    if (it->userId == aUserId)
    {
      if(0 == --quote)
      {
        mBuyOrders.erase(it);
        std::make_heap(mBuyOrders.begin(), mBuyOrders.end());
        return "Success!\n";
      }
    }
  }

  for (auto it = mSellOrders.begin(); it != mSellOrders.end(); ++it)
  {
    if (it->userId == aUserId)
    {
      if(--quote == 0)
      {
        mSellOrders.erase(it);
        std::make_heap(mSellOrders.begin(), mSellOrders.end());
        return "Success!\n";
      }
    }
  }

  return "Could not find quote " + aQuote + '\n';
}

