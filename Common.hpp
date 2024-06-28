#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <string>

static short port = 5555;

namespace Requests
{
    static std::string Registration = "Reg";
    static std::string TopUpBalance = "Top";
    static std::string Balance      = "Bal";
    static std::string BuyOrder     = "Buy";
    static std::string SellOrder    = "Sel";
    static std::string QuotesInfo   = "Quo";
    static std::string Trades       = "Tra";
}

#endif //CLIENSERVERECN_COMMON_HPP
