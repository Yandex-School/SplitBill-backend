#ifndef JWT_UTILS_H
#define JWT_UTILS_H

#include <jwt-cpp/jwt.h>
#include <string>
#include <exception>
#include <iostream>
#include <optional>
#include <userver/server/http/http_request.hpp>

namespace split_bill {

struct TSession;

class JwtUtils {
 public:
  static std::string createToken(const std::string& sessionId, const std::string& userId);
  static bool verifyToken(const std::string& token);
  static std::optional<TSession> GetToken(const userver::server::http::HttpRequest& request);
};

}  // namespace split_bill

#endif // JWT_UTILS_H