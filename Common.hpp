#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <string>

static short port = 5555;

namespace Requests
{
    static std::string Registration = "Reg";
    static std::string Balance      = "Bal";
    static std::string BuyOrder     = "Buy";
    static std::string SellOrder    = "Sel";
    static std::string ActiveQuotes = "Quo";
    static std::string Trades       = "Tra";

    static std::string Cancel       = "Can";
}

#endif //CLIENSERVERECN_COMMON_HPP
