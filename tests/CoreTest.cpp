#include <gtest/gtest.h>

#include "../Core.hpp"

class CoreTest : public ::testing::Test
{
  protected:
    Core core;

    void SetUp() override
    {
      core.RegisterNewUser("User1");
    }
};

TEST_F(CoreTest, RegisterNewUser)
{
  EXPECT_EQ(core.GetUserName("0"), "User1");
  auto usrId = core.RegisterNewUser("User2");
  EXPECT_EQ(core.GetUserName(usrId), "User2");

  usrId = core.RegisterNewUser("User3");
  EXPECT_EQ(core.GetUserName(usrId), "User3");
}

TEST_F(CoreTest, GetUserName)
{
  auto usrId = core.RegisterNewUser("User2");
  EXPECT_EQ(core.GetUserName(usrId), "User2");

  EXPECT_EQ(core.GetUserName("11"), "Error! Unknown User\n");
}

TEST_F(CoreTest, GetUserBalance1)
{
  auto usrId = core.RegisterNewUser("User123");
  EXPECT_EQ(core.GetUserName(usrId), "User123");

  EXPECT_EQ(core.GetUserBalance(usrId), "RUB 0\nUSD 0\n");
}

TEST_F(CoreTest, GetUserBalance2)
{
  auto usrId = core.RegisterNewUser("User user");
  EXPECT_EQ(core.GetUserName(usrId), "User user");

  EXPECT_EQ(core.GetUserBalance("1100"), "Error! Unknown User\n");
}

TEST_F(CoreTest, PlaceNewOrder1)
{
  auto usrId = core.RegisterNewUser("User 4");
  EXPECT_EQ(core.GetUserName(usrId), "User 4");

  // заявка на покупку
  EXPECT_EQ(core.PlaceNewOrder(usrId, "100", "62.5", true),
      "Your order was succesfully placed.\n");
}

TEST_F(CoreTest, PlaceNewOrder2)
{
  auto usrId = core.RegisterNewUser("User 5");
  EXPECT_EQ(core.GetUserName(usrId), "User 5");

  // заявка на покупку
  EXPECT_EQ(core.PlaceNewOrder("111", "100", "62.5", true),
      "Error! Unknown User\n");
}

TEST_F(CoreTest, PlaceNewOrder3)
{
  auto usrId = core.RegisterNewUser("User 0");
  EXPECT_EQ(core.GetUserName(usrId), "User 0");

  // заявка на покупку
  EXPECT_EQ(core.PlaceNewOrder(usrId, "0.0", "62.5", true),
      "Error. Incorrect USD amount.\n");
}

TEST_F(CoreTest, PlaceNewOrder4)
{
  auto usrId = core.RegisterNewUser("User 1");
  EXPECT_EQ(core.GetUserName(usrId), "User 1");

  // заявка на покупку
  EXPECT_EQ(core.PlaceNewOrder(usrId, "0.5", "-0.8", true),
      "Error. Incorrect USD price.\n");
}

TEST_F(CoreTest, PlaceNewOrder5)
{
  auto usrId_1 = core.RegisterNewUser("User p1");
  EXPECT_EQ(core.GetUserName(usrId_1), "User p1");

  auto usrId_2 = core.RegisterNewUser("User p2");
  EXPECT_EQ(core.GetUserName(usrId_2), "User p2");

  auto usrId_3 = core.RegisterNewUser("User p3");
  EXPECT_EQ(core.GetUserName(usrId_3), "User p3");

  EXPECT_EQ(core.PlaceNewOrder(usrId_1, "10", "62", true),
      "Your order was succesfully placed.\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_2, "20", "63", true),
      "Your order was succesfully placed.\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_3, "50", "61", false),
      "Your order was succesfully placed.\n");
  
  EXPECT_EQ(core.GetUserBalance(usrId_1), "RUB -620\nUSD 10\n");
  EXPECT_EQ(core.GetUserBalance(usrId_2), "RUB -1260\nUSD 20\n");
  EXPECT_EQ(core.GetUserBalance(usrId_3), "RUB 1880\nUSD -30\n");

  EXPECT_EQ(core.GetUserActiveQuotes(usrId_1), "You have no active quotes.\n");
  EXPECT_EQ(core.GetUserActiveQuotes(usrId_2), "You have no active quotes.\n");
  EXPECT_EQ(core.GetUserActiveQuotes(usrId_3),
      "1) " + usrId_3 + " 20 61 SELL\n");
}

TEST_F(CoreTest, PlaceNewOrder6)
{
  auto usrId_1 = core.RegisterNewUser("User 2");
  EXPECT_EQ(core.GetUserName(usrId_1), "User 2");

  EXPECT_EQ(core.PlaceNewOrder(usrId_1, "100", "62.5", true),
      "Your order was succesfully placed.\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_1, "50", "63", true),
      "Your order was succesfully placed.\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_1, "25", "65", true),
      "Your order was succesfully placed.\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_1, "175", "66", false),
      "Your order was succesfully placed.\n");
  
  auto usrId_2 = core.RegisterNewUser("User -1");
  EXPECT_EQ(core.GetUserName(usrId_2), "User -1");

  EXPECT_EQ(core.PlaceNewOrder(usrId_2, "145", "62.45", false),
      "Your order was succesfully placed.\n");

  // купит 25 usd по 65
  // потом 50 usd по 63
  // затем 70 usd по 62.5
  //
  // Итого 25*65 + 50*63 + 70*62.5 = 1625 + 3150 + 4375 = 9150 RUB за 145 USD
  
  EXPECT_EQ(core.GetUserBalance(usrId_1), "RUB -9150\nUSD 145\n");
  EXPECT_EQ(core.GetUserBalance(usrId_2), "RUB 9150\nUSD -145\n");

  EXPECT_EQ(core.GetUserActiveQuotes(usrId_1),
      "1) " + usrId_1 + " 30 62.5 BUY\n"
      "2) " + usrId_1 + " 175 66 SELL\n");
  EXPECT_EQ(core.GetUserActiveQuotes(usrId_2), "You have no active quotes.\n");
}

TEST_F(CoreTest, PlaceNeOrder7)
{
  auto usrId_1 = core.RegisterNewUser("User 2");
  EXPECT_EQ(core.GetUserName(usrId_1), "User 2");

  auto usrId_3 = core.RegisterNewUser("User 3");
  EXPECT_EQ(core.GetUserName(usrId_3), "User 3");

  auto usrId_4 = core.RegisterNewUser("User 4");
  EXPECT_EQ(core.GetUserName(usrId_4), "User 4");

  EXPECT_EQ(core.PlaceNewOrder(usrId_1, "100", "62.5", false),
      "Your order was succesfully placed.\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_4, "25", "65", false),
      "Your order was succesfully placed.\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_3, "50", "65", false),
      "Your order was succesfully placed.\n");
  
  auto usrId_2 = core.RegisterNewUser("User -1");
  EXPECT_EQ(core.GetUserName(usrId_2), "User -1");

  EXPECT_EQ(core.PlaceNewOrder(usrId_2, "145", "65", true),
      "Your order was succesfully placed.\n");

  // покупает сначала 100 usd по 62.5 (usr1)
  // потом покупает 25 usd по 65 (usr4)
  // и покупает 20 usd по 65 (usr3)

  // 6250 + 1625 + 1300 = 9175
  EXPECT_EQ(core.GetUserBalance(usrId_2), "RUB -9175\nUSD 145\n");
  EXPECT_EQ(core.GetUserBalance(usrId_4), "RUB 1625\nUSD -25\n");
  EXPECT_EQ(core.GetUserBalance(usrId_3), "RUB 1300\nUSD -20\n");
  EXPECT_EQ(core.GetUserBalance(usrId_1), "RUB 6250\nUSD -100\n");

  EXPECT_EQ(core.GetUserActiveQuotes(usrId_3),
      "1) " + usrId_3 + " 30 65 SELL\n");
  EXPECT_EQ(core.GetUserActiveQuotes(usrId_1), "You have no active quotes.\n");
  EXPECT_EQ(core.GetUserActiveQuotes(usrId_2), "You have no active quotes.\n");
  EXPECT_EQ(core.GetUserActiveQuotes(usrId_4), "You have no active quotes.\n");

  EXPECT_EQ(core.GetUserTrades("228"), "Error! Unknown User\n");
  EXPECT_EQ(core.GetUserTrades(usrId_2),
      usrId_1 + " SOLD " + usrId_2 + " 100 USD for 62.5 RUB\n" +
      usrId_4 + " SOLD " + usrId_2 + " 25 USD for 65 RUB\n" +
      usrId_3 + " SOLD " + usrId_2 + " 20 USD for 65 RUB\n");
}

TEST_F(CoreTest, GetUserActiveQuotes1)
{
  auto usrId_1 = core.RegisterNewUser("User 11");
  EXPECT_EQ(core.GetUserName(usrId_1), "User 11");

  EXPECT_EQ(core.GetUserActiveQuotes("11"), "Error! Unknown User\n");
  EXPECT_EQ(core.PlaceNewOrder("11", "100", "62.5", false),
      "Error! Unknown User\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_1, "100", "62.5", false),
      "Your order was succesfully placed.\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_1, "100", "100", true),
      "Your order was succesfully placed.\n");
  EXPECT_EQ(core.PlaceNewOrder(usrId_1, "100", "65.5", false),
      "Your order was succesfully placed.\n");

  EXPECT_EQ(core.GetUserActiveQuotes(usrId_1),
      "1) " + usrId_1 + " 100 100 BUY\n"
      "2) " + usrId_1 + " 100 62.5 SELL\n"
      "3) " + usrId_1 + " 100 65.5 SELL\n");

  EXPECT_EQ(core.CancelUserQuote(usrId_1, "0"), "Incorrect Quote number.\n");
  EXPECT_EQ(core.CancelUserQuote(usrId_1, "4"), "Could not find quote 4\n");
  EXPECT_EQ(core.CancelUserQuote(usrId_1, "1"), "Success!\n");

  EXPECT_EQ(core.GetUserActiveQuotes(usrId_1),
      "1) " + usrId_1 + " 100 62.5 SELL\n"
      "2) " + usrId_1 + " 100 65.5 SELL\n");

  EXPECT_EQ(core.CancelUserQuote(usrId_1, "2"), "Success!\n");

  EXPECT_EQ(core.CancelUserQuote("35", "2"), "Error! Unknown User\n");

  EXPECT_EQ(core.GetUserActiveQuotes(usrId_1),
      "1) " + usrId_1 + " 100 62.5 SELL\n");
}
