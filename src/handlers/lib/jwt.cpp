#include "jwt.h"
#include "../../models/session.hpp"
#include <chrono>

namespace split_bill {

const std::string JWT_ISSUER = "issuer";
const std::string JWT_SECRET = "secret";

const int JWT_EXPIRATION = 3600; //in seconds

std::string JwtUtils::createToken(const std::string& sessionId, const std::string& userId) {
  try {
    auto now = std::chrono::system_clock::now();
    auto expiration = std::chrono::system_clock::to_time_t(
        now + std::chrono::seconds(JWT_EXPIRATION)
    );

    auto token = jwt::create()
                     .set_type("HS256")
                     .set_issuer(JWT_ISSUER)
                     .set_payload_claim("sessionId", jwt::claim(sessionId))
                     .set_payload_claim("userId", jwt::claim(userId))
                     .set_expires_at(std::chrono::system_clock::from_time_t(expiration))
                     .sign(jwt::algorithm::hs256{JWT_SECRET});
    return token;
  } catch (const std::exception& e) {
    std::cerr << "Error creating token: " << e.what() << std::endl;
    throw;
  }
}

bool JwtUtils::verifyToken(const std::string& token) {
  try {
    auto decoded = jwt::decode(token);

    auto verifier = jwt::verify()
                        .with_issuer(JWT_ISSUER)
                        .allow_algorithm(jwt::algorithm::hs256{JWT_SECRET});

    verifier.verify(decoded);
    std::cout << "Token verification successful!" << std::endl;
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Token verification failed: " << e.what() << std::endl;
    return false;
  }
}

std::optional<TSession> JwtUtils::GetToken(const userver::server::http::HttpRequest& request) {
  auto auth_header = request.GetHeader("Authorization");
  auto prefix = auth_header.substr(0, 7);
  if (auth_header.empty() || prefix != "Bearer ") {
    return std::nullopt;
  }

  std::string token = auth_header.substr(7);

  // Verify token and extract claims
  if (!verifyToken(token)) {
    return std::nullopt;
  }

  auto decoded = jwt::decode(token);
  TSession session;
  session.token = token;
  session.user_id = std::stoi(decoded.get_payload_claim("userId").as_string());
  return session;
}

}  // namespace split_bill